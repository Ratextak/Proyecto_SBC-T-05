// Para hacer el siguiente script se ha utilizado como base el siguiente:
/* 
 * esp-idf-telegram-bot
 * Author: antusystem
 * e-mail: aleantunes95@gmail.com
 * Date: 11-01-2020
 * MIT License
 * As it is described in the readme file
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "credentials.h"

#define MAX_HTTP_OUTPUT_BUFFER 1024  // Tamaño máx del buffer de respuesta HTTP.
char buffer_respuesta[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer para guardar la respuesta.
char url[100];  // Url que se formará para los POST y GET.

esp_http_client_handle_t cliente_http;  // Cliente HTTP.
esp_err_t mensaje_error;

/* TAGs for the system*/
static const char *TAG0 = "HTTP_CLIENT Handler";
static const char *TAG1 = "wifi station";
static const char *TAG2 = "Sending getMe";
static const char *TAG3 = "Sending sendMessage";

// WIFI configuration.
#define ESP_MAXIMUM_RETRY  10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

extern const char telegram_certificate_pem_start[] asm("_binary_telegram_certificate_pem_start");
extern const char telegram_certificate_pem_end[]   asm("_binary_telegram_certificate_pem_end");


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG1, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG1,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG1, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_NAME,
            .password = WIFI_PASSWORD,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG1, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG1, "connected to ap SSID:%s password:%s",
                 WIFI_NAME, WIFI_PASSWORD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG1, "Failed to connect to SSID:%s, password:%s",
                 WIFI_NAME, WIFI_PASSWORD);
    } else {
        ESP_LOGE(TAG1, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG0, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG0, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG0, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG0, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG0, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG0, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG0, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG0, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG0, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                ESP_LOGI(TAG0, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG0, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

void http_app_start(void){
    esp_http_client_config_t config = {
        .url = TELEGRAM_SERVER,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = telegram_certificate_pem_start,
        .user_data = buffer_respuesta,
    };

    ESP_LOGW(TAG0, "Iniciamos cliente HTTP.");
    cliente_http = esp_http_client_init(&config);
}

static void https_telegram_getMe_perform(void) {
	char url[100];

    // Preparamos y fijamos el url.
    sprintf(url, "%s/getme", BOT_URL);
    esp_http_client_set_url(cliente_http, url);

    esp_http_client_set_method(cliente_http, HTTP_METHOD_GET);
    mensaje_error = esp_http_client_perform(cliente_http);

    if (mensaje_error == ESP_OK) {
        ESP_LOGI(TAG2, "HTTPS Status = %d, content_length = %d",
                esp_http_client_get_status_code(cliente_http),
                esp_http_client_get_content_length(cliente_http));
        ESP_LOGW(TAG2, "Desde Perform el output es: %s", buffer_respuesta);
    } else
        ESP_LOGE(TAG2, "Error perform http request %s", esp_err_to_name(mensaje_error));

    esp_http_client_close(cliente_http);
    esp_http_client_cleanup(cliente_http);
}

static void https_telegram_sendMessage_perform_post(char mensaje[]) {

    /* Formato para mandar mensajes:
	https://api.telegram.org/bot[BOT_TOKEN]/sendMessage?chat_id=[CHAT_ID]&text=[MESSAGE_TEXT]
    The format for the json is: {"chat_id":852596694,"text":"Message using post"}
	*/

    // Preparamos y fijamos el url.
    sprintf(url, "%s/sendMessage", BOT_URL);
    esp_http_client_set_url(cliente_http, url);

	ESP_LOGW(TAG3, "Enviare POST");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "chat_id", BOT_CHAT_ID);
    cJSON_AddStringToObject(root, "text", mensaje);
    
    char *post_data = cJSON_PrintUnformatted(root);
    ESP_LOGW(TAG3, "El json es es: %s", post_data);
    esp_http_client_set_method(cliente_http, HTTP_METHOD_POST);
    esp_http_client_set_header(cliente_http, "Content-Type", "application/json");
    esp_http_client_set_post_field(cliente_http, post_data, strlen(post_data));

    mensaje_error = esp_http_client_perform(cliente_http);
    if (mensaje_error == ESP_OK) {
        ESP_LOGI(TAG3, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(cliente_http),
                esp_http_client_get_content_length(cliente_http));
        ESP_LOGW(TAG3, "Desde Perform el output es: %s", buffer_respuesta);

    } else
        ESP_LOGE(TAG3, "HTTP POST request failed: %s", esp_err_to_name(mensaje_error));

    ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
    cJSON_Delete(root);
    free(post_data);
}

void iniciar_http(void) {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG1, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    http_app_start();
}
