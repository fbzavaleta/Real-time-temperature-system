/*
 * Lib C
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/*
 * Drivers
 */
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "dht.h"
#include "nvs_flash.h"

#include "modules/buzzer.h"
#include "modules/state.h"
#include "modules/led.h"
#include "modules/network.h"

/*
 * Logs
 */
#include "esp_log.h"

/*
 * DHT11
 */
#define DHT_SENSOR_TYPE DHT_TYPE_DHT11
#define DHT_DATA_GPIO GPIO_NUM_27
#define DHT_READ_DELAY 50

// Struct dos dados a serem enviados ao ThingSpeaks
typedef struct thingSpeaksData
{
    int sock;
    float temperature;
    float difference;
    bool isHeartbeat;
    bool isTemperature;
} thingSpeaksData_t;

// Headers para enviar os dados ao ThingSpeaks
const char *thingSpeaksPost =
    "POST /update HTTP/1.1\n"
    "Host: api.thingspeak.com\n"
    "Connection: close\n"
    "X-THINGSPEAKAPIKEY: %s\n"
    "Content-Type: application/x-www-form-urlencoded\n"
    "content-length: ";

static const char *TAG = "TEMP";

// Filas que irão armazenar os dados de temperatura e heartbeat a serem enviados
QueueHandle_t xQueueSendTemperature;
QueueHandle_t xQueueSendHeartbeat;

// Fila responsável por enviar a diferença de temperatura a função que calcula o estado da temperatura
QueueHandle_t xQueueProcessState;

// Fila respponsável por alterar a IHM quando há alteração no estado da temperatura
QueueHandle_t xQueueUpdateState;

// Struct utilizado para enviar temperatura e variação com as filas
typedef struct temperatureVariation
{
    float temperature;
    float difference;
} temperatureVariation_t;

// Temperatura inicial do componente, é feita uma leitura na inicialização
float initialTemperature;

// TASK - Responsável por enviar um heartbeat (1) para a fila a cada 100ms
void heartbeat_send(void *pvParameter)
{

    int heartbeat = 1;

    for (;;)
    {
        // Adiciona o Heartbeat na fila de envio
        if (xQueueSend(xQueueSendHeartbeat, &heartbeat, portMAX_DELAY) != pdPASS)
        {
            ESP_LOGI(TAG, "Falha ao tentar adicionar o Heartbeat na Queue 'Send Heartbeat'!");
        }

        // Aguardar 100ms para enviar o próximo heartbeat
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}

// TASK - Responsável por enviar os dados ao ThingSpeak através de HTTP
void http_sendThingSpeaks(void *pvParameter)
{
    int total = 1 * 1024;
    char post_string[200];
    const char *apikey = &THINGSPEAKS_API_WRITE_KEY[0];

    char *buffer = pvPortMalloc(total);
    if (buffer == NULL)
    {
        ESP_LOGI(TAG, "pvPortMalloc Error\r\n"); // alocate block of memory from the heap
        vTaskDelete(NULL);
        return;
    }
    // Dados à serem enviados, obtido através do pvParameter
    thingSpeaksData_t *data = (thingSpeaksData_t *)pvParameter;
    sprintf(post_string, thingSpeaksPost, apikey);

    char fields[40] = "";
    // Inclusão dos fields de temperatura, caso seja um dado que inclua as informações
    if (data->isTemperature)
    {
        sprintf(fields, "&field1=%.1f&field2=%.1f", data->temperature, data->difference);
    }
    // Inclusão do field de hearbeat, caso seja um dado que possua o heartbeat
    if (data->isHeartbeat)
    {
        strcat(fields, "&field3=1");
    }

    // Corpo da mensagem que será enviada ao ThingSpeaks
    char databody[70];
    sprintf(databody, "{%s%s}", THINGSPEAKS_API_WRITE_KEY, fields);
    sprintf(buffer, "%s%d\r\n\r\n%s\r\n\r\n", post_string, strlen(databody), databody);

    ESP_LOGI(TAG, "ThingSpeaks - Enviando: %s", databody);

    // Envia os dados através do Socket aberto
    send(data->sock, buffer, strlen(buffer), 0);
    // Acabamos removendo a leitura da resposta do HTTP pois não estava vendo utilizada e possuia um alto custo de processamento
    close(data->sock);

    vPortFree(buffer);
    vTaskDelete(NULL);
}

// TASK - Responsável por monitorar as filas de envio de temperatura e heartbeat, caso seja necessário, chamar o envio via HTTP
void thingSpeaks_send(void *pvParameter)
{
    int rc;

    // Dados recebidos das filas de temperatura e heartbeat
    temperatureVariation_t temperature_variation;
    bool heartbeat;

    // Struct que será enviado via HTTP
    thingSpeaksData_t thingSpeaksData;

    for (;;)
    {
        // Tenta obter um item das filas de Heartbeat e Temperatura, e guarda nas variaveis se conseguiu ou não receber
        bool temperatureReceived = xQueueReceive(xQueueSendTemperature, &temperature_variation, 0) == pdTRUE;
        bool heartbeatReceived = xQueueReceive(xQueueSendHeartbeat, &heartbeat, 0) == pdTRUE;

        // Caso tenha recebido um item de qualquer uma das filas
        if (temperatureReceived || heartbeatReceived)
        {
            // Faz a abertura do Socket de comunicação, para envio dos dados
            int sock;
            open_socket(&sock, &rc);

            if (rc == -1)
            {
                ESP_LOGI(TAG, "ThingSpeaks - Error on Socket: %d", rc);
                for (int i = 1; i <= 5; ++i)
                {
                    ESP_LOGI(TAG, "ThingSpeaks - Timeout: %d", 5 - i);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
                continue;
            }

            // Se a conexão tiver sido estabelecida com sucesso
            if (rc != -1)
            {
                // Constrói o struct para realizar o envio dos dados via HTTP
                thingSpeaksData.sock = sock;
                thingSpeaksData.temperature = temperature_variation.temperature;
                thingSpeaksData.difference = temperature_variation.difference;
                thingSpeaksData.isHeartbeat = heartbeatReceived;
                thingSpeaksData.isTemperature = temperatureReceived;

                // Cria uma nova task para executar o envio dos dados via HTTP
                xTaskCreate(http_sendThingSpeaks, "http_sendThingSpeaks", 16000, (void *)&(thingSpeaksData), 5, NULL);
            }
        }
    }

    vTaskDelete(NULL);
}

/*
 * Temperature - Leitura Inicial
 * Essa função foi criada para executar a leitura de temperatura 5 vezes
 * Utilizamos essa 5 leituras e executamos a média afim de remover algum "vies" caso fosse somente uma leitura
 */
void read_initial_temperature()
{
    // Variavel para acumular as temperaturas lidas e o numero de leituras que será realizada
    float temperatureSum = 0;
    int reads = 5;

    for (int i = 0; i < reads;)
    {
        float temperature;
        // Caso a leitura do sensor DHT seja bem sucedida, acumula o valor lido na variavel
        if (dht_read_float_data(DHT_SENSOR_TYPE, DHT_DATA_GPIO, NULL, &temperature) == ESP_OK)
        {
            temperatureSum = temperatureSum + temperature;
            i++;
        }
    }

    // Realiza a divisão das temperaturas observadas, em função do número de leituras, afim de obter a média
    initialTemperature = temperatureSum / reads;

    ESP_LOGI(TAG, "Temperatura inicial: %.1fC\n", initialTemperature);
}

/*
 * Temperature - Leitura do sensor
 * - Realiza a leitura do sensor DHT
 * - Calcula a diferença em relação a temperatura inicial
 * - Envia ambas as informações para as filas de processamento
 */
void dht_read(void *pvParameters)
{
    // Setup do GPIO onde está posicionado o DHT11
    gpio_pad_select_gpio(DHT_DATA_GPIO);
    gpio_set_direction(DHT_DATA_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT_DATA_GPIO, GPIO_PULLUP_ONLY);

    // Aguardar alguns millisegundos para iniciar a execução das leituras
    vTaskDelay(pdMS_TO_TICKS(DHT_READ_DELAY * 10));

    for (;;)
    {
        float current_temperature;

        // Tenta executar a leitura da temperatura através do sensor DHT, caso consiga, prossegue
        if (dht_read_float_data(DHT_SENSOR_TYPE, DHT_DATA_GPIO, NULL, &current_temperature) == ESP_OK)
        {

            // Calcula a diferença em relação a temperatura inicial
            float temperatureDifference = current_temperature - initialTemperature;
            if (temperatureDifference != 0)
            {
                ESP_LOGI(TAG, "Diferença da Temperatura: %.1fC\n", temperatureDifference);
            }

            // Monta o struct com a diferença e temperatura lida, para enviar a fila
            temperatureVariation_t variation;
            variation.temperature = current_temperature;
            variation.difference = temperatureDifference;

            // Adiciona na fila que irá processar qual o estado de temperatura
            if (xQueueSend(xQueueProcessState, &temperatureDifference, portMAX_DELAY) != pdPASS)
            {
                ESP_LOGI(TAG, "Falha ao tentar adicionar a temperatura na Queue 'Process State'!");
            }

            // Adiciona na fila que irá enviar os dados ao ThingSpeaks
            if (xQueueSend(xQueueSendTemperature, &variation, portMAX_DELAY) != pdPASS)
            {
                ESP_LOGI(TAG, "Falha ao tentar adicionar a temperatura na Queue 'Send Temperature'!");
            }

            // Aguarda alguns millisegundos para executar uma próxima leitura
            vTaskDelay(pdMS_TO_TICKS(DHT_READ_DELAY));
        }
        else
        {
            ESP_LOGI(TAG, "Falha ao tentar realizar a leitura do sensor DHT11");

            vTaskDelay(pdMS_TO_TICKS(DHT_READ_DELAY * 2));
        }
    }

    vTaskDelete(NULL);
}

/*
 * Temperature - Processa o estado atual da temperatura
 * - Utilizando a diferença de temperatura informada, calcula qual deve ser o estado
 * - Caso haja uma mudança no estado, adiciona na fila para atualizar a IHM
 */
void temperature_state()
{
    float temperature_difference = 0;

    // Estado da temperatura observado no ciclo de execução anterior
    int last_state = -1;
    // Quantas vezes o estado de temperatura está sendo observado consecutivamente
    int consecutive_state = 0;

    // Ultimo estado que foi enviado as filas de atualização de estado
    int last_notified_state = -1;

    for (;;)
    {
        // Tenta obter um item da fila de processamento de estado da temperatura
        xQueueReceive(xQueueProcessState, &temperature_difference, portMAX_DELAY);
        // Para este cenário, só importa o quanto a temperatura variou, aqui obtemos o "modulo" da diferenca
        if (temperature_difference < 0)
        {
            temperature_difference = -temperature_difference;
        }

        int current_state = -1;
        // Caso a variação tenha sido igual ou inferior a 2, o estado da temperatura será "Verde"
        if (temperature_difference <= 2)
        {
            current_state = TEMPERATURE_STATE_GREEN;
        }
        // Caso a variação tenha sido igual ou inferior a 5, o estado da temperatura será "Laranja"
        else if (temperature_difference <= 5)
        {
            current_state = TEMPERATURE_STATE_ORANGE;
        }
        // Caso a variação tenha sido superior a 2, o estado da temperatura será "Vermelho"
        else
        {
            current_state = TEMPERATURE_STATE_RED;
        }

        // Caso tenha sido obtido algum estado de temperatura
        if (current_state != -1)
        {
            // Caso o estado de temperatura atual, seja igual ao anteriormente lido
            if (last_state == current_state)
            {
                // Incrementa o número de leituras com o estado da temperatuar consecutivos
                consecutive_state++;

                // O estado da temperatura deve ser diferente do ultimo enviado a fila
                if (last_notified_state != current_state)
                {
                    // Validar o estado da temperatura com o numero de leituras necessarias
                    if (current_state == TEMPERATURE_STATE_GREEN ||
                        (current_state == TEMPERATURE_STATE_ORANGE && consecutive_state >= 10) ||
                        (current_state == TEMPERATURE_STATE_RED && consecutive_state >= 15))
                    {
                        // Informar a fila de atualizacao de estado que ocorreu a alteracao
                        if (xQueueSend(xQueueUpdateState, &current_state, portMAX_DELAY) != pdPASS)
                        {
                            ESP_LOGI(TAG, "Falha ao tentar adicionar o estado na Queue 'Update State'!");
                        }

                        last_notified_state = current_state;
                    }
                }
            }
            else
            {
                // Caso o estado de temperatura seja diferente do anterior, altera a variavel e zera o contador de estados consecutivos
                last_state = current_state;
                consecutive_state = 0;
            }
        }
    }

    vTaskDelete(NULL);
}

/*
 * IHM - Atualizar os estados do LED RGB e Buzzer
 * - Cancela a task que estava executando do LED RGB e inicia uma nova, devido a mudança de estado
 * - Cancela a task que estava executando do BUZZER e inicia uma nova, devido a mudança de estado
 */
void update_state()
{
    int current_state = -1;

    // Variáveis que irão armazenar as tasks em execução do LED RGB e Buzzer
    TaskHandle_t taskHandle_ledState = NULL;
    TaskHandle_t taskHandle_buzzerState = NULL;

    for (;;)
    {
        // Tenta obter um item da fila de atualização de estado de temperatura
        xQueueReceive(xQueueUpdateState, &current_state, portMAX_DELAY);

        // Cancela a execução da Task atual do LED RGB
        if (taskHandle_ledState != NULL)
        {
            vTaskDelete(taskHandle_ledState);
            taskHandle_ledState = NULL;
        }

        // Cancela a execução da Task atual do Buzzer
        if (taskHandle_buzzerState != NULL)
        {
            vTaskDelete(taskHandle_buzzerState);
            taskHandle_buzzerState = NULL;
        }

        ESP_LOGI(TAG, "O estado foi alterado para %i", current_state);

        // Inicia a execução da nova Task do LED RGB, passando o estado via pvParameter
        xTaskCreate(led_state, "led_state", 10000, (void *)&(current_state), 10, &taskHandle_ledState);

        // Inicia a execução da nova Task do BUZZER, passando o estado via pvParameter
        xTaskCreate(buzzer_state, "buzzer_state", 10000, (void *)&(current_state), 10, &taskHandle_buzzerState);
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_config();

    led_setup();
    buzzer_setup();

    read_initial_temperature();

    if ((xQueueProcessState = xQueueCreate(10, sizeof(float))) == NULL)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Queue 'Process State'.\n");
        return;
    }

    if ((xQueueSendTemperature = xQueueCreate(10, sizeof(temperatureVariation_t))) == NULL)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Queue 'Send Temperature'.\n");
        return;
    }

    if ((xQueueSendHeartbeat = xQueueCreate(10, sizeof(int))) == NULL)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Queue 'Send Heartbeat'.\n");
        return;
    }

    if ((xQueueUpdateState = xQueueCreate(10, sizeof(int))) == NULL)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Queue 'Update State'.\n");
        return;
    }

    if ((xTaskCreate(dht_read, "dht_read", 2048, NULL, 1, NULL)) != pdTRUE)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Task 'dht_read'.\n");
        return;
    }

    if ((xTaskCreate(temperature_state, "temperature_state", 2048, NULL, 2, NULL)) != pdTRUE)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Task 'temperature_state'.\n");
        return;
    }

    if ((xTaskCreate(update_state, "update_state", 2048, NULL, 5, NULL)) != pdTRUE)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Task 'update_state'.\n");
        return;
    }

    if ((xTaskCreate(heartbeat_send, "heartbeat_send", 2048, NULL, 5, NULL)) != pdTRUE)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Task 'heartbeat_send'.\n");
        return;
    }

    if ((xTaskCreate(thingSpeaks_send, "thingSpeaks_send", 2048, NULL, 5, NULL)) != pdTRUE)
    {
        ESP_LOGI(TAG, "Não foi possível alocar a Task 'thingSpeaks_send'.\n");
        return;
    }
}