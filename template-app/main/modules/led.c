/*
 * "Modulo LED RGB"
 * Responsável por:
 * - Configurar os canais e timers PWM para cada um dos pinos
 * - Alterar a cor do Led entre Verde, Laranja e Vermelho baseado no estado da temperatura, usando PWM
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "state.h"
#include "led.h"

/*
 * Configurar os canais e timers do PWM para cada um dos pinos do LED RGB
 */
void led_setup(void)
{
    // Configuração do Timer - Red
    ledc_timer_config_t ledc_timer_red = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LED_RED_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT, // min =0 / max = 2^8 -1
        .freq_hz = 50,                       // t = 1/50
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer_red);

    // Configuração do Channel - Red
    ledc_channel_config_t ledc_channel_red = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LED_RED_LEDC_CHANNEL,
        .timer_sel = LED_RED_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_RED_PWM_GPIO,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel_red);

    // Configuração do Timer - Green
    ledc_timer_config_t ledc_timer_green = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LED_GREEN_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT, // min =0 / max = 2^8 -1
        .freq_hz = 50,                       // t = 1/50
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer_green);

    // Configuração do Channel - Green
    ledc_channel_config_t ledc_channel_green = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LED_GREEN_LEDC_CHANNEL,
        .timer_sel = LED_GREEN_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_GREEN_PWM_GPIO,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel_green);

    // Configuração do Timer - Blue
    ledc_timer_config_t ledc_timer_blue = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LED_BLUE_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT, // min =0 / max = 2^8 -1
        .freq_hz = 50,                       // t = 1/50
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer_blue);

    // Configuração do Channel - Blue
    ledc_channel_config_t ledc_channel_blue = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LED_BLUE_LEDC_CHANNEL,
        .timer_sel = LED_BLUE_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_BLUE_PWM_GPIO,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel_blue);
}

/*
 * Recebe o estado atual da temperatura através dos pvParameters e altera a cor do LED
 * Executado através da criação de uma task
 */
void led_state(void *pvParameters)
{
    // Parar o DUTY do canal Vermelho do LED RGB
    ledc_stop(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL, 0);
    // Parar o DUTY do canal Verde do LED RGB
    ledc_stop(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL, 0);
    // Parar o DUTY do canal Azul do LED RGB
    ledc_stop(LEDC_HIGH_SPEED_MODE, LED_BLUE_LEDC_CHANNEL, 0);

    // Obtem o estado da temperatura a partir do pvParameters
    int currentState = *(int *)pvParameters;

    for (;;)
    {
        if (currentState == TEMPERATURE_STATE_GREEN)
        {
            // Alterar o DUTY do canal Verde do LED RGB para 255 (Máximo)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL, 255);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(2000));

            // Alterar o DUTY do canal Verde do LED RGB para 0 (Desligado)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL, 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        else if (currentState == TEMPERATURE_STATE_ORANGE)
        {
            // Alterar o DUTY do canal Verde do LED RGB para 60 (Para misturar com vermelho, para obter o laranja)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL, 60);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL);
            // Alterar o DUTY do canal Vermelho do LED RGB para 255 (Máximo)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL, 255);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(200));

            // Alterar o DUTY do canal Verde do LED RGB para 0 (Desligado)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL, 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_GREEN_LEDC_CHANNEL);
            // Alterar o DUTY do canal Vermelho do LED RGB para 0 (Desligado)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL, 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (currentState == TEMPERATURE_STATE_RED)
        {
            // Alterar o DUTY do canal Vermelho do LED RGB para 255 (Máximo)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL, 255);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(50));

            // Alterar o DUTY do canal Vermelho do LED RGB para 0 (Desligado)
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL, 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LED_RED_LEDC_CHANNEL);
            // Aguardar o delay
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    // Caso ocorra algum problema, encerrar o task
    vTaskDelete(NULL);
}