/*
 * "Modulo LED RGB"
 * - Definições dos GPIos para cada canal do LED
 * - Definições dos canais e timers para cada canal do LED
 */


#include "driver/gpio.h"
#include "driver/ledc.h"

// GPIOs de cada canal do LED RGB
#define LED_RED_PWM_GPIO GPIO_NUM_32
#define LED_BLUE_PWM_GPIO GPIO_NUM_33
#define LED_GREEN_PWM_GPIO GPIO_NUM_25

// Canal do ledc para cada canal do LED RGB
#define LED_RED_LEDC_CHANNEL LEDC_CHANNEL_1
#define LED_BLUE_LEDC_CHANNEL LEDC_CHANNEL_2
#define LED_GREEN_LEDC_CHANNEL LEDC_CHANNEL_3

// Timer do ledc para cada canal do LED RGB
#define LED_RED_LEDC_TIMER LEDC_TIMER_1
#define LED_BLUE_LEDC_TIMER LEDC_TIMER_2
#define LED_GREEN_LEDC_TIMER LEDC_TIMER_3

/*
 * Configurar os canais e timers do PWM para cada um dos pinos do LED RGB
 */
extern void led_setup(void);

/*
 * Recebe o estado atual da temperatura através dos pvParameters e altera a cor do LED
 */
extern void led_state(void *pvParameters);