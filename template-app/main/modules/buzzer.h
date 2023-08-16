/*
 * "Modulo Buzzer"
 * - Definições do GPIo para o PIN "Data"
 * - Definições do canal e timer para o PWM
 */


#include "driver/gpio.h"
#include "driver/ledc.h"

// GPIO do "Data" do Buzzer
#define BUZZER_PWM_GPIO GPIO_NUM_26

// Dutys de ativação e desativação do PWM
#define BUZZER_PWM_OFF 256
#define BUZZER_PWM_ON 64

// Timer e canal LEDC para o Buzzer
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_LEDC_TIMER LEDC_TIMER_0

/*
 * Configurar o canl e tmer do PWM para o "Data do GPIO"
 */
extern void buzzer_setup(void);

/*
 * Recebe o estado atual da temperatura através dos pvParameters e altera a ativação do Buzzer
 */
extern void buzzer_state(void *pvParameters);