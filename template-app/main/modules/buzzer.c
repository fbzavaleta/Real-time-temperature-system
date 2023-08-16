/*
 * "Modulo Buzzer"
 * Responsável por:
 * - Configurar o canal e timer PWM para o Buzzer
 * - Alterar de ativação do Buzzer baseado no estado da temperatura, usando PWM
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "state.h"
#include "buzzer.h"

/*
 * Configurar o canl e tmer do PWM para o "Data do GPIO"
 */
void buzzer_setup(void)
{
    // configuração do timer
    ledc_timer_config_t ledc_timer_buzzer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = BUZZER_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT, // min =0 / max = 2^8 -1
        .freq_hz = 50,                       // t = 1/50
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer_buzzer);

    // configuração do channel
    ledc_channel_config_t ledc_channel_buzzer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .timer_sel = BUZZER_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_PWM_GPIO,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel_buzzer);
}

/*
 * Recebe o estado atual da temperatura através dos pvParameters e altera a ativação do Buzzer
 */
void buzzer_state(void *pvParameters)
{
    // Parar o DUTY do Buzzers
    ledc_stop(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL, 1);

    // Obtem o estado da temperatura a partir do pvParameters
    int currentState = *(int *)pvParameters;

    for (;;)
    {
        if (currentState == TEMPERATURE_STATE_GREEN)
        {
            // Alterar o DUTY do Buzzer para Desligado
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_PWM_OFF);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else if (currentState == TEMPERATURE_STATE_ORANGE)
        {
            // Alterar o DUTY do Buzzer para Máximo
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_PWM_ON);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(200));

            // Alterar o DUTY do Buzzer para Desligado
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_PWM_OFF);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (currentState == TEMPERATURE_STATE_RED)
        {
            // Alterar o DUTY do Buzzer para Máximo
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_PWM_ON);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(70));

            // Alterar o DUTY do Buzzer para Desligado
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_PWM_OFF);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(70));
        }
    }

    vTaskDelete(NULL);
}
