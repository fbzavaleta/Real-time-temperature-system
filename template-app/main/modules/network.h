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
#define THINGSPEAKS_SERVER_IP "184.106.153.149" 
#define THINGSPEAKS_SERVER_PORT 80 
#define THINGSPEAKS_API_WRITE_KEY "286MIKXXNF27OEEE"

/*
wifi credencials
*/
#define EXAMPLE_ESP_WIFI_SSID 	"Vuelma" 
#define EXAMPLE_ESP_WIFI_PASS 	"vuel789456" 

/*
Function avaliable from this module
*/
extern void wifi_config( void );
extern void  open_socket(int * sock_var, int * status_var);