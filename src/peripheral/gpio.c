#include "gpio.h"

void gpio_init()
{
    // PA0  - AF (TIM2CC)       - Rival310 Scroll RGB Green
    // PA1  - AF (TIM2CC)       - Rival310 Scroll RGB Red
    // PA2  - AF (TIM2CC)       - Rival310 Scroll RGB Blue
    // PA3  - GP (IRQ)          - TrueMove3 Motion
    // PA4  - GP (Chip select)  - TrueMove3(NCS) Flash(CS)
    // PA5  - AF (SCK)          - TrueMove3 + Flash SCK
    // PA6  - AF (MISO)         - TrueMove3 + Flash MISO
    // PA7  - AF (MOSI)         - TrueMove3 + Flash MOSI
    // PA8  - AF (TIM1CC)       - Rival 310 Logo RGB Green
    // PA9  - AF (TIM1CC)       - Rival 310 Logo RGB Red
    // PA10 - AF (TIM1CC)       - Rival 310 Logo RGB Blue
    // PA11 -                   - Rival 310 USB DM
    // PA12 -                   - Rival 310 USB DP
    // PA13 - AF (SWDIO) Output 50MHz push-pull
    // PA14 - AF (SWCLK) Output 50MHz push-pull
    // PA15 -
    RCC->APB2RSTR |= RCC_APB2RSTR_IOPARST; // Reset peripheral
    RCC->APB2RSTR &= ~RCC_APB2RSTR_IOPARST;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA peripheral clock
    GPIOA->CRL = IO_MODE_CFG(0,  IO_MODE_OUT_10M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(1,  IO_MODE_OUT_10M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(2,  IO_MODE_OUT_10M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(3,  IO_MODE_IN,      IO_CFG_IN_PULL)   |
                 IO_MODE_CFG(4,  IO_MODE_OUT_10M, IO_CFG_OUT_GP_PP) |
                 IO_MODE_CFG(5,  IO_MODE_OUT_50M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(6,  IO_MODE_IN,      IO_CFG_IN_FLOAT)  |
                 IO_MODE_CFG(7,  IO_MODE_OUT_50M, IO_CFG_OUT_AF_PP);
    GPIOA->CRH = IO_MODE_CFG(8,  IO_MODE_OUT_10M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(9,  IO_MODE_OUT_10M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(10, IO_MODE_OUT_10M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(13, IO_MODE_OUT_50M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(14, IO_MODE_OUT_50M, IO_CFG_OUT_AF_PP);
    GPIOA->ODR = 0;

    // PB0 - GP (IO)        - Rival310 Left Button
    // PB1 - GP (IO)        - Rival310 Scroll Button
    // PB2 - GP (IO)        - Rival310 Right Button
    // PB3 - AF (SWO) Output 50MHz push-pull    - Rival310 DPI Button
    // PB4 - AF (TIM3CH1(Remaped))   - Rival310 Encoder A
    // PB5 - AF (TIM3CH2(Remaped))   - Rival310 Encoder B
    // PB6 - GP (IO)        - Rival310 Back Button
    // PB7 - GP (IO)        - Rival310 Forward Button
    RCC->APB2RSTR |= RCC_APB2RSTR_IOPBRST; // Reset peripheral
    RCC->APB2RSTR &= ~RCC_APB2RSTR_IOPBRST;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // Enable GPIOB peripheral clock
    GPIOB->CRL = IO_MODE_CFG(0, IO_MODE_IN,      IO_CFG_IN_PULL)   |
                 IO_MODE_CFG(1, IO_MODE_IN,      IO_CFG_IN_PULL)   |
                 IO_MODE_CFG(2, IO_MODE_IN,      IO_CFG_IN_PULL)   |
                 IO_MODE_CFG(3, IO_MODE_OUT_50M, IO_CFG_OUT_AF_PP) |
                 IO_MODE_CFG(4, IO_MODE_IN,      IO_CFG_IN_PULL)   |
                 IO_MODE_CFG(5, IO_MODE_IN,      IO_CFG_IN_PULL)   |
                 IO_MODE_CFG(6, IO_MODE_IN,      IO_CFG_IN_PULL)   |
                 IO_MODE_CFG(7, IO_MODE_IN,      IO_CFG_IN_PULL);
    GPIOB->CRH = 0;
    GPIOB->ODR = 0;

    /*
    // PC13 - Blue-Pill LED
    RCC->APB2RSTR |= RCC_APB2RSTR_IOPCRST; // Reset peripheral
    RCC->APB2RSTR &= ~RCC_APB2RSTR_IOPCRST;
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // Enable GPIOC peripheral clock
    GPIOC->CRL = 0;
    GPIOC->CRH = IO_MODE_CFG(13, IO_MODE_OUT_2M, IO_CFG_OUT_GP_PP);
    GPIOC->ODR = 0;
    */

    // PD0 -
    // PD1 -
    RCC->APB2RSTR |= RCC_APB2RSTR_IOPDRST; // Reset peripheral
    RCC->APB2RSTR &= ~RCC_APB2RSTR_IOPDRST;
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN; // Enable GPIOD peripheral clock
    GPIOD->CRL = 0;
    GPIOD->CRH = 0;
    GPIOD->ODR = 0;

    // External Interrupts
    RCC->APB2RSTR |= RCC_APB2RSTR_AFIORST; // Reset peripheral
    RCC->APB2RSTR &= ~RCC_APB2RSTR_AFIORST;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AFIO peripheral clock
    AFIO->EXTICR[0] = IO_EXTI_CFG(3, IO_EXTI_PORTA);
    AFIO->EXTICR[1] = 0;
    AFIO->EXTICR[2] = 0;
    AFIO->EXTICR[3] = 0;

    // EXTI 3 - Motion in
    EXTI->IMR |= EXTI_IMR_MR3; // Enable interrupt
    EXTI->EMR |= EXTI_EMR_MR3; // Enable event
    EXTI->FTSR |= EXTI_FTSR_TR3; // Enable falling trigger
    EXTI->RTSR &= ~EXTI_RTSR_TR3; // Disable rising trigger
    IRQ_SET_PRIO(EXTI3_IRQn, 1, 0);
    IRQ_CLEAR(EXTI3_IRQn);
    IRQ_ENABLE(EXTI3_IRQn);

    // Remapping
    AFIO->MAPR = AFIO_MAPR_SWJ_CFG_JTAGDISABLE | AFIO_MAPR_TIM3_REMAP_PARTIALREMAP; // Disable JTAG and remap TIM3
}