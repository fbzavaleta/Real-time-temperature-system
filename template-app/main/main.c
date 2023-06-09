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
#include "freertos/task.h"
#include "freertos/queue.h"
/*
 * Drivers
 */
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include <dht.h>
/*

Actividade - due 11/05

Modificar o código para o envio das variaveis de temperatura e humedidade para o envio do
thinkspeaks.
*/

#if defined(CONFIG_EXAMPLE_TYPE_DHT11)
#define SENSOR_TYPE DHT_TYPE_DHT11
#endif
#if defined(CONFIG_EXAMPLE_TYPE_AM2301)
#define SENSOR_TYPE DHT_TYPE_AM2301
#endif
#if defined(CONFIG_EXAMPLE_TYPE_SI7021)
#define SENSOR_TYPE DHT_TYPE_SI7021
#endif


/*
 * logs
 */
#include "esp_log.h"


static const char * TAG = "MAIN-NAV: ";

void http_Socket ( void * pvParameter );
void http_SendReceive ( void * pvParameter );
#define DEGUG 1
#define pwm_pin 18

#define buzzer_off 0
#define buzzer_on  230

typedef struct xData {
 	int sock; 
 	uint32_t val_teste;
} xSocket_t;

const char * msg_post = \

	"POST /update HTTP/1.1\n"
	"Host: api.thingspeak.com\n"
	"Connection: close\n"
	"X-THINGSPEAKAPIKEY: %s\n"
	"Content-Type: application/x-www-form-urlencoded\n"
	"content-length: ";


void http_Socket(void * pvParameter)
{
	int rc; 
	xSocket_t xSocket;
	
	for (;;)
	{
		int sock;

		open_socket(&sock, &rc);
		ESP_LOGI(TAG, "Status Socket: %d", rc);

		if (rc == -1)
		{
			ESP_LOGI(TAG, "error on Socket: %d", rc);
			for( int i = 1; i <= 5; ++i )
			{	
				ESP_LOGI(TAG, "timeout: %d", 5-i);
				vTaskDelay( 1000/portTICK_PERIOD_MS );
			}
			continue;			
		}

		xSocket.sock = sock;
		xSocket.val_teste = 10;

		xTaskCreate( http_SendReceive, "http_SendReceive", 10000, (void*)&(xSocket), 5, NULL );
	}
	vTaskDelete(NULL);
	
}

void http_SendReceive(void * pvParameter)
{
	int rec_offset = 0; 
	int total =	1*1024; 
	char post_string[200];
	const char *apikey = &API_WRITE_KEY[0];

	char *buffer = pvPortMalloc( total );
	if( buffer == NULL ) 
	{
		
		ESP_LOGI(TAG, "pvPortMalloc Error\r\n"); //alocate block of memory from the heap
		vTaskDelete(NULL); 	  
		return;
	 }
    xSocket_t * xSocket = (xSocket_t*) pvParameter;
	sprintf(post_string, msg_post, apikey);

	char databody[50];
  	sprintf( databody, "{%s&field1=%d}", API_WRITE_KEY, xSocket->val_teste); //modificar a estructure xdata
	sprintf( buffer , "%s%d\r\n\r\n%s\r\n\r\n", post_string, strlen(databody), databody);
  
	int rc = send( xSocket->sock, buffer, strlen(buffer), 0 );

	ESP_LOGI(TAG, "HTTP Enviado? rc: %d", rc);
	
	for(;;)
	{
		ssize_t sizeRead = recv(xSocket->sock, buffer+rec_offset, total-rec_offset, 0);
		
		if ( sizeRead == -1 ) 
		{
			ESP_LOGI( TAG, "recv: %d", sizeRead );
		}

		if ( sizeRead == 0 ) 
		{
			break;
		}

		if( sizeRead > 0 ) 
		{	
			ESP_LOGI(TAG, "Socket: %d - Data read (size: %d) was: %.*s", xSocket->sock, sizeRead, sizeRead, buffer);
		   
		   rec_offset += sizeRead;
		 }

		vTaskDelay( 5/portTICK_PERIOD_MS );
	}
	
	rc = close(xSocket->sock);
	
	ESP_LOGI(TAG, "close: rc: %d", rc); 
	
	vPortFree( buffer );	

	vTaskDelete(NULL); 	
}

//configuração do PWM

void pwm_config()
{
	//configuração do timer
	ledc_timer_config_t ledc_timer ={
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_num  = LEDC_TIMER_0,
		.duty_resolution = LEDC_TIMER_8_BIT, // min =0 / max = 2^8 -1
		.freq_hz = 50, //t = 1/50
		.clk_cfg = LEDC_AUTO_CLK
	};
	ledc_timer_config(&ledc_timer);

	//configuração do channel
	ledc_channel_config_t ledc_channel = {
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.channel =    LEDC_CHANNEL_0,
		.timer_sel = LEDC_TIMER_0,
		.intr_type = LEDC_INTR_DISABLE,
		.gpio_num = pwm_pin,
		.duty = 0,
		.hpoint = 0
	};
	ledc_channel_config(&ledc_channel);
}

//função do dutty
void send_pwm()
{
	for (;;)
	{
		//enviando sinal buzzer ->on
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, buzzer_on);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
		vTaskDelay(100/portTICK_PERIOD_MS);
		ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0,0);

		//enviando sinal buzzer ->off
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, buzzer_off);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
		vTaskDelay(100/portTICK_PERIOD_MS);
		ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0,0);		
	}
	
}

void dht_test(void *pvParameters)
{
    float temperature, humidity;

#ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
    gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);
#endif

    while (1)
    {
        if (dht_read_float_data(SENSOR_TYPE, CONFIG_EXAMPLE_DATA_GPIO, &humidity, &temperature) == ESP_OK)
            printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);
        else
            printf("Could not read data from sensor\n");

        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
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
	pwm_config();

	
    if( ( xTaskCreate( http_Socket, "http_Socket", 2048, NULL, 5, NULL )) != pdTRUE )
	{
		ESP_LOGI( TAG, "error - nao foi possivel alocar http_Socket.\n" );	
		return;		
	}   

    if( ( xTaskCreate( send_pwm, "send_pwm", 2048, NULL, 5, NULL )) != pdTRUE )
	{
		ESP_LOGI( TAG, "error - nao foi possivel alocar send_pwm.\n" );	
		return;		
	}  
	
    if( ( xTaskCreate( dht_test, "dht_test", 2048, NULL, 5, NULL )) != pdTRUE )
	{
		ESP_LOGI( TAG, "error - nao foi possivel alocar dht_test.\n" );	
		return;		
	}  	 

}