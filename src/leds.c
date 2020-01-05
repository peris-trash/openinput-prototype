#include "leds.h"


/* holds information about a led instance */
typedef struct {
    uint32_t mode; // upper 8 bits = mode, lower 16 bits = period, leftover is reserved for now
    color_t color;
    int* redChannel;
    int* GreenChannel;
    int* BlueChannel;
} led_t;

led_t leds[NUM_LEDS];

void leds_init()
{
    RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST; // Reset peripheral
    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // Enable TIM1 peripheral clock

    while(TIM1->CR1 & TIM_CR1_CEN)
        TIM1->CR1 &= ~TIM_CR1_CEN; // Disable the timer

    // timer configuration
    TIM1->ARR = (1 << 8) - 1; // 8-bit PWM
    TIM1->PSC = 6; // frequency = [APB1_TIM_CLOCK_FREQ / TIM1->PSC + 1 / TIM1->ARR] = [36MHz / 7 / 255] =  20.168KHz
    TIM1->EGR |= TIM_EGR_UG; // Update immediatly

    // capture compare configuration
    TIM1->CCMR1 = (6 << 4) | (6 << 12); // PWM Mode 1 (CH1 & CH2)
    TIM1->CCMR2 = (6 << 4); // PWM Mode 1 (CH3)
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E; // Enable CH1, CH2, CH3, active high

    // capture compare values
    TIM1->CCR1 = 0; // capture compare ch1, sets green duty cycle (0-255)
    TIM1->CCR2 = 0; // capture compare ch2, sets red duty cycle (0-255)
    TIM1->CCR3 = 0; // capture compare ch3, sets blue duty cycle (0-255)

    TIM1->BDTR = TIM_BDTR_MOE; // Main output enable

    TIM1->CR1 |= TIM_CR1_CEN; // enable timer

    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST; // Reset peripheral
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 peripheral clock

    while(TIM2->CR1 & TIM_CR1_CEN)
        TIM2->CR1 &= ~TIM_CR1_CEN; // Disable the timer

    // timer configuration
    TIM2->ARR = (1 << 8) - 1; // 8-bit PWM
    TIM2->PSC = 6; // frequency = [APB1_TIM_CLOCK_FREQ / TIM1->PSC + 1 / TIM1->ARR] = [36MHz / 7 / 255] =  20.168KHz
    TIM2->EGR |= TIM_EGR_UG; // Update immediatly

    // capture compare configuration
    TIM2->CCMR1 = (6 << 4) | (6 << 12); // PWM Mode 1 (CH1 & CH2)
    TIM2->CCMR2 = (6 << 4); // PWM Mode 1 (CH3)
    TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E; // Enable CH1, CH2, CH3, active high

    // capture compare values
    TIM2->CCR1 = 0; // capture compare ch1, sets green duty cycle (0-255)
    TIM2->CCR2 = 0; // capture compare ch2, sets red duty cycle (0-255)
    TIM2->CCR3 = 0; // capture compare ch3, sets blue duty cycle (0-255)

    TIM2->BDTR = TIM_BDTR_MOE; // Main output enable

    TIM2->CR1 |= TIM_CR1_CEN; // enable timer

    leds[0].mode = 0x00000000;
    leds[0].color.r = 0x00;
    leds[0].color.g = 0x00;
    leds[0].color.b = 0x00;
	leds[0].redChannel = (int*)&TIM1->CCR2;
    leds[0].GreenChannel = (int*)&TIM1->CCR1;
    leds[0].BlueChannel = (int*)&TIM1->CCR3;

    leds[1].mode = 0x00000000;
    leds[1].color.r = 0x00;
    leds[1].color.g = 0x00;
    leds[1].color.b = 0x00;
	leds[1].redChannel = (int*)&TIM2->CCR1;
    leds[1].GreenChannel = (int*)&TIM2->CCR2;
    leds[1].BlueChannel = (int*)&TIM2->CCR3;
}
void leds_task()
{
    for(uint8_t i = 0; i < NUM_LEDS; i++) // cycle through leds, try to achieve target color
    {
        static color_t current_color[NUM_LEDS] = {};

        if(current_color[i].r < leds[i].color.r) current_color[i].r += 1;
		if(current_color[i].r > leds[i].color.r) current_color[i].r -= 1;

		if(current_color[i].g < leds[i].color.g) current_color[i].g += 1;
		if(current_color[i].g > leds[i].color.g) current_color[i].g -= 1;

		if(current_color[i].b < leds[i].color.b) current_color[i].b += 1;
		if(current_color[i].b > leds[i].color.b) current_color[i].b -= 1;

		*leds[i].redChannel = current_color[i].r;
		*leds[i].GreenChannel = current_color[i].g;
		*leds[i].BlueChannel = current_color[i].b;
    }
}
/* modify led instance */
void leds_set(uint8_t led, uint8_t mode, uint16_t period, color_t color)
{
    if((led < 0) || (led >= NUM_LEDS)) return;

    leds[led].mode = (mode << 24) | period;
    leds[led].color = color;
}