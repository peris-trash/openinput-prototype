#include "systick.h"

volatile uint64_t g_ullSystemTick = 0;

void _systick_isr()
{
    g_ullSystemTick++;
}
void systick_init()
{
    // delay ms initialization
    SysTick->LOAD = (SYS_CLOCK_FREQ / 8 / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    SCB->SHP[11] = 15 << (8 - __NVIC_PRIO_BITS); // Set priority 7,1 (min)

    // delay us initialization
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable TRC
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;    // Enable  clock cycle counter
}
void delay_ms(uint32_t ullTicks)
{
    NONATOMIC_BLOCK(NONATOMIC_RESTORESTATE)
    {
        uint64_t ullStartTick = g_ullSystemTick;

        while((g_ullSystemTick - ullStartTick) < ullTicks);
    }
}
void delay_us(uint32_t ullTicks)
{
    uint32_t startTick = DWT->CYCCNT;
    ullTicks *= (SYS_CLOCK_FREQ / 1000000);

    while(DWT->CYCCNT - startTick < ullTicks);
}
