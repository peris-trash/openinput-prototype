#include <stm32f10x.h>
#include <stdlib.h>
#include <math.h>
#include "dbg.h"
#include "debug_macros.h"
#include "nvic.h"
#include "utils.h"
#include "atomic.h"
#include "systick.h"
#include "rcc.h"
#include "gpio.h"
#include "spi.h"
#include "crc.h"
#include "flash.h"
#include "leds.h"
#include "spi_eeprom.h"
#include "truemove3.h"

// Structs
typedef struct
{
    volatile uint16_t usID0_15;
    volatile uint16_t usID16_31;
    volatile uint32_t ulID32_63;
    volatile uint32_t ulID64_96;
} system_unique_id_t;

// Internal Flash Addresses
#define FLASH_APP_ADDRESS       (FLASH_BASE + 0x8000)

// Forward declarations
static void reset() __attribute__((noreturn));
static void sleep();

static uint32_t get_free_ram();

// Variables
extern system_unique_id_t _system_unique_id;

// ISRs
void _nmi_isr()
{
    // The NMI is used for the Clock Security System (CSS)
    rcc_update_clocks();

    dbg_swo_config(BIT(0), 6000000); // Init Debug module // Init SWO channels 0 at 1 MHz

    systick_init();

    spi_init(SPI_MODE3, SPI_CLOCK_DIV_64, SPI_MSB_FIRST);

    DBGPRINTLN_CTX("HSE Clock failed, switched to HSI!");
    DBGPRINTLN_CTX("RCC - System Clock: %.1f MHz!", (float)SYS_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - AHB Clock: %.1f MHz!", (float)AHB_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB1 Clock: %.1f MHz!", (float)APB1_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB2 Clock: %.1f MHz!", (float)APB2_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB1 Timers Clock: %.1f MHz!", (float)APB1_TIM_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB2 Timers Clock: %.1f MHz!", (float)APB2_TIM_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - ADC Clock: %.1f MHz!", (float)ADC_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("SPI - Clock: %.1f MHz!", (float)APB2_CLOCK_FREQ / 1000000 / 2);

    RCC->CIR |= RCC_CIR_CSSC;

    DBGPRINTLN_CTX("Resetting system in 2s...");

    delay_ms(2000);

    reset();
}
void _exti3_isr()
{
    if(EXTI->PR & EXTI_PR_PR3)
    {
        EXTI->PR = EXTI_PR_PR3;

        truemove3_motion_event();
    }
}

// Functions
void reset()
{
    SCB->AIRCR = 0x05FA0000 | _VAL2FLD(SCB_AIRCR_SYSRESETREQ, 1);

    while(1);
}
void sleep()
{
    DBGPRINTLN_CTX("Sleeping peripherals...");

    SCB->SCR |= _VAL2FLD(SCB_SCR_SLEEPDEEP, 1); // CPU deep sleep
    PWR->CR &= ~PWR_CR_PDDS; // Stop mode
    PWR->CR |= PWR_CR_LPDS; // Regulator in Low-power mode

    DBGPRINTLN_CTX("Going to sleep after mem transactions");

    delay_ms(20);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        __DMB(); // Wait for all memory transactions to finish before memory access
        __DSB(); // Wait for all memory transactions to finish before executing instructions
        __ISB(); // Wait for all memory transactions to finish before fetching instructions
        __SEV(); // Set the event flag to ensure the next WFE will be a NOP
        __WFE(); // NOP and clear the event flag
        __WFE(); // Wait for event
        __NOP(); // Prevent debugger crashes

        rcc_init(); // Re-configure clocks as we wake up running on HSI

        DBGPRINTLN_CTX("Just woke up, running ISR...");
    }

    DBGPRINTLN_CTX("Finished wakeup ISR");

    DBGPRINTLN_CTX("Waking up peripherals...");
}

uint32_t get_free_ram()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        extern void *_sbrk(int);

        void *pCurrentHeap = _sbrk(1);

        if(!pCurrentHeap)
            return 0;

        uint32_t ulFreeRAM = (uint32_t)__get_MSP() - (uint32_t)pCurrentHeap;

        _sbrk(-1);

        return ulFreeRAM;
    }
}

int init()
{
    rcc_init(); // Switch to HSE/PLL
    rcc_update_clocks(); // Update clock values

    dbg_swo_config(BIT(0), 6000000); // Init Debug module // Init SWO channels 0 at 1 MHz

    systick_init(); // Init system tick

    gpio_init(); // Init GPIOs

    spi_init(SPI_MODE3, SPI_CLOCK_DIV_64, SPI_MSB_FIRST);


    DBGPRINTLN_CTX("openinput v%lu (%s %s)!", BUILD_VERSION, __DATE__, __TIME__);
    DBGPRINTLN_CTX("Interfaces init OK!");
    DBGPRINTLN_CTX("Device ID: 0x%03X", DBGMCU->IDCODE & DBGMCU_IDCODE_DEV_ID);
    DBGPRINTLN_CTX("Device Revision: 0x%04X", (DBGMCU->IDCODE & DBGMCU_IDCODE_REV_ID) >> 16);
    DBGPRINTLN_CTX("Flash Size: %hu kB", FLASH_SIZE >> 10);
    DBGPRINTLN_CTX("Free RAM: %lu B", get_free_ram());
    DBGPRINTLN_CTX("Unique ID: %04X-%04X-%08X-%08X", _system_unique_id.usID0_15, _system_unique_id.usID16_31, _system_unique_id.ulID32_63, _system_unique_id.ulID64_96);

    DBGPRINTLN_CTX("RCC - System Clock: %.1f MHz!", (float)SYS_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - AHB Clock: %.1f MHz!", (float)AHB_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB1 Clock: %.1f MHz!", (float)APB1_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB2 Clock: %.1f MHz!", (float)APB2_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB1 Timers Clock: %.1f MHz!", (float)APB1_TIM_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB2 Timers Clock: %.1f MHz!", (float)APB2_TIM_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("SPI - Clock: %.1f MHz!", (float)APB2_CLOCK_FREQ / 1000000 / 64);

    delay_ms(50);

    uint8_t* truemove3Firmware = malloc(4094);
    for(uint16_t b = 0; b < 4094; b += 128)
    {
        spi_eeprom_read(0x6100 + b, truemove3Firmware + b, 128);
    }

    uint8_t sromId = truemove3_init(truemove3Firmware);
    free(truemove3Firmware);
    if(sromId)
    {
        DBGPRINTLN_CTX("TRUEMOVE3 initialized with srom ver:0x%02X!", sromId);
    }
    else
    {
        DBGPRINTLN_CTX("TRUEMOVE3 Failed initialization!");
        while(1);
    }

    leds_init();

    return 0;
}
int main()
{
    #ifdef DUMPEEPROM
    do
    {
        DBGPRINTLN_CTX("Dumping eeprom contents...");
        DBGPRINTLN("[address : value]");
        uint16_t eepromaddr = 0x0000;
        uint8_t contents[128] = {};
        for(uint16_t p = 0; p < 512; p++)
        {
            spi_eeprom_read(eepromaddr, contents, 128);
            for(uint8_t i = 0; i < 128; i++)
            {
                DBGPRINTLN("0x%04X, 0x%02X", eepromaddr++, contents[i]);
            }
            delay_ms(50);
        }
        DBGPRINTLN_CTX("Dumping complete...");
    }
    while(0);
    #endif

    while(1)
    {
		static uint64_t lastDeltaPrint = 0;
        if(g_ullSystemTick > (lastDeltaPrint + 1000))
        {
            delta_xy_t deltas = truemove3_get_deltas();
            DBGPRINTLN("delta X:%d Delta Y:%d", deltas.x, deltas.y);
            lastDeltaPrint = g_ullSystemTick;
        }
    }

    return 0;
}
