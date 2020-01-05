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

// Structs
typedef struct
{
    volatile uint16_t usID0_15;
    volatile uint16_t usID16_31;
    volatile uint32_t ulID32_63;
    volatile uint32_t ulID64_96;
} system_unique_id_t;

// Internal Flash Addresses
#define FLASH_APP_ADDRESS           (FLASH_BASE + 0x8000)

// Forward declarations
static void reset() __attribute__((noreturn));
static void sleep();

static uint32_t get_free_ram();

// Variables
extern system_unique_id_t _system_unique_id;

const uint8_t ubLights[360] = { // sine fade technique to cycle rgb led
  0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 15, 17, 18, 20, 22, 24, 26, 28, 30, 32, 35, 37, 39,
 42, 44, 47, 49, 52, 55, 58, 60, 63, 66, 69, 72, 75, 78, 81, 85, 88, 91, 94, 97, 101, 104, 107, 111, 114, 117, 121, 124, 127, 131, 134, 137,
141, 144, 147, 150, 154, 157, 160, 163, 167, 170, 173, 176, 179, 182, 185, 188, 191, 194, 197, 200, 202, 205, 208, 210, 213, 215, 217, 220, 222, 224, 226, 229,
231, 232, 234, 236, 238, 239, 241, 242, 244, 245, 246, 248, 249, 250, 251, 251, 252, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 253,
252, 251, 251, 250, 249, 248, 246, 245, 244, 242, 241, 239, 238, 236, 234, 232, 231, 229, 226, 224, 222, 220, 217, 215, 213, 210, 208, 205, 202, 200, 197, 194,
191, 188, 185, 182, 179, 176, 173, 170, 167, 163, 160, 157, 154, 150, 147, 144, 141, 137, 134, 131, 127, 124, 121, 117, 114, 111, 107, 104, 101, 97, 94, 91,
 88, 85, 81, 78, 75, 72, 69, 66, 63, 60, 58, 55, 52, 49, 47, 44, 42, 39, 37, 35, 32, 30, 28, 26, 24, 22, 20, 18, 17, 15, 13, 12,
 11, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


// ISRs
void _nmi_isr()
{
    // The NMI is used for the Clock Security System (CSS)
    rcc_update_clocks();

    dbg_swo_config(BIT(0), 6000000); // Init Debug module // Init SWO channels 0 at 1 MHz

    systick_init();

//    spi1_init(0, SPI_CLOCK_DIV_2, SPI_MSB_FIRST);

    DBGPRINTLN_CTX("HSE Clock failed, switched to HSI!");
    DBGPRINTLN_CTX("RCC - System Clock: %.1f MHz!", (float)SYS_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - AHB Clock: %.1f MHz!", (float)AHB_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB1 Clock: %.1f MHz!", (float)APB1_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB2 Clock: %.1f MHz!", (float)APB2_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB1 Timers Clock: %.1f MHz!", (float)APB1_TIM_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - APB2 Timers Clock: %.1f MHz!", (float)APB2_TIM_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("RCC - ADC Clock: %.1f MHz!", (float)ADC_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("UART1 - Baud: %lu bps!", 500000);
    DBGPRINTLN_CTX("SPI1 - Clock: %.1f MHz!", (float)APB2_CLOCK_FREQ / 1000000 / 2);
    DBGPRINTLN_CTX("I2C2 - Clock: %lu kHz!", 100);

    RCC->CIR |= RCC_CIR_CSSC;

    DBGPRINTLN_CTX("Resetting system in 2s...");

    delay_ms(2000);

    reset();
}
void _exti0_isr()
{
    if(EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR = EXTI_PR_PR0;

//        rfm69_isr();
    }
}
void _exti15_10_isr()
{
    if(EXTI->PR & EXTI_PR_PR12)
    {
        EXTI->PR = EXTI_PR_PR12;


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

    systick_init(); // Init system tick

    gpio_init(); // Init GPIOs

    leds_init();

    spi_init(SPI_MODE0, SPI_CLOCK_DIV_2, SPI_MSB_FIRST);


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
    DBGPRINTLN_CTX("RCC - ADC Clock: %.1f MHz!", (float)ADC_CLOCK_FREQ / 1000000);
    DBGPRINTLN_CTX("SPI1 - Clock: %.1f MHz!", (float)APB2_CLOCK_FREQ / 1000000 / 2);

    delay_ms(50);

    return 0;
}
int main()
{
    for(;;)
    {
		static color_t red = {.r = 255, .g = 0, .b = 0};
		static color_t green = {.r = 0, .g = 255, .b = 0};
		static color_t blue = {.r = 0, .g = 0, .b = 255};
		static color_t off = {.r = 0, .g = 0, .b = 0};

		leds_set(0, 0, 0, red);
		leds_set(1, 0, 0, green);
		for (uint8_t i = 0; i < 255; i++)
		{
			for (int ii = 0; ii < 128000; ii++) __asm__("nop");
			leds_task();
		}

		leds_set(0, 0, 0, green);
		leds_set(1, 0, 0, blue);
		for (uint8_t i = 0; i < 255; i++)
		{
			for (int ii = 0; ii < 128000; ii++) __asm__("nop");
			leds_task();
		}

		leds_set(0, 0, 0, blue);
		leds_set(1, 0, 0, red);
		for (uint8_t i = 0; i < 255; i++)
		{
			for (int ii = 0; ii < 128000; ii++) __asm__("nop");
			leds_task();
		}

		leds_set(0, 0, 0, off);
		leds_set(1, 0, 0, off);
		for (uint8_t i = 0; i < 255; i++)
		{
			for (int ii = 0; ii < 128000; ii++) __asm__("nop");
			leds_task();
		}
    }

    return 0;
}