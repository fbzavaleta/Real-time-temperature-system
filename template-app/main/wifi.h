#include "esp_log.h"
#include <lwip/sockets.h>

/*
Debug
*/
#undef  ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x)   do { esp_err_t rc = (x); if (rc != ESP_OK) { ESP_LOGE("err", "esp_err_t = %d", rc); assert(0 && #x);} } while(0);

/*
API 
*/
#define SERVER_IP "184.106.153.149" 
#define SERVER_PORT 80 
#define API_WRITE_KEY "4YQSYIDJPSCMDZDM"

/*
wifi credencials
*/
#define EXAMPLE_ESP_WIFI_SSID 	"francis5G" 
#define EXAMPLE_ESP_WIFI_PASS 	"12345678" 

/*
Function avaliable from this module
*/
extern void wifi_config( void );
extern void  open_socket(int * sock_var, int * status_var);