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

static const char *TAG_HTTP = "HTTP_Telegram";

#define MAX_HTTP_OUTPUT_BUFFER 2048  // Tamaño máx del buffer de respuesta HTTP.
char buffer_respuesta[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer para guardar la respuesta.
char url[150];  // Url que se formará para los POST y GET.

esp_http_client_handle_t cliente_http;  // Cliente HTTP.
esp_err_t mensaje_error;

int ultimo_updateID = 0;

// WIFI configuration.
#define ESP_MAXIMUM_RETRY  10

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

// Certificado del Telegram.
extern const char telegram_certificate_pem_start[] asm("_binary_telegram_certificate_pem_start");
extern const char telegram_certificate_pem_end[]   asm("_binary_telegram_certificate_pem_end");


esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG_HTTP, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG_HTTP, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG_HTTP, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG_HTTP, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG_HTTP, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
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
                            ESP_LOGE(TAG_HTTP, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG_HTTP, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG_HTTP, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_HTTP, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                ESP_LOGI(TAG_HTTP, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG_HTTP, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

void http_app_start(void) {
    esp_http_client_config_t config = {
        .url = TELEGRAM_SERVER,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = telegram_certificate_pem_start,
        .user_data = buffer_respuesta,
    };

    ESP_LOGW(TAG_HTTP, "Iniciamos cliente HTTP.");
    cliente_http = esp_http_client_init(&config);
}

static void https_telegram_getUpdates(char *respuesta) {
    // Preparamos y fijamos el url.
    sprintf(url, "%s/getUpdates?offset=%d&limit=1", BOT_URL, ultimo_updateID+1);
    esp_http_client_set_url(cliente_http, url);

    ESP_LOGI(TAG_HTTP, "Recibiré GET");
    esp_http_client_set_method(cliente_http, HTTP_METHOD_GET);

    mensaje_error = esp_http_client_perform(cliente_http);
    if (mensaje_error == ESP_OK) {
        ESP_LOGI(TAG_HTTP, "HTTPS Status = %d, content_length = %d",
                esp_http_client_get_status_code(cliente_http),
                esp_http_client_get_content_length(cliente_http));
        ESP_LOGW(TAG_HTTP, "Output del GET: %s", buffer_respuesta);
    } else
        ESP_LOGE(TAG_HTTP, "Error perform http request %s", esp_err_to_name(mensaje_error));
    
    cJSON *json_devuelto = cJSON_Parse(buffer_respuesta);
    cJSON *result = cJSON_GetObjectItem (json_devuelto, "result");
    cJSON *resultArray = cJSON_GetArrayItem (result, 0);
    if (resultArray != NULL) {
        cJSON *updateID = cJSON_GetObjectItem (resultArray, "update_id");
        if (updateID->valueint != ultimo_updateID) {
            ultimo_updateID = updateID->valueint;
            cJSON *message = cJSON_GetObjectItem (resultArray, "message");
            cJSON *text = cJSON_GetObjectItem (message, "text");
            if (text != NULL) {
                for (int i=0; i <10; i++)
                    respuesta[i] = text->valuestring[i];
            } else
                *respuesta = NULL;
        } else
            *respuesta = NULL;
    } else 
        *respuesta = NULL;
    
    memset(buffer_respuesta, NULL, MAX_HTTP_OUTPUT_BUFFER);
    cJSON_Delete(json_devuelto);
}

static void https_telegram_sendMessage(char mensaje[]) {
    // Preparamos y fijamos el url.
    sprintf(url, "%s/sendMessage", BOT_URL);
    esp_http_client_set_url(cliente_http, url);

    // Preparamos el JSON con el mensaje a enviar.
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "chat_id", BOT_CHAT_ID);
    cJSON_AddStringToObject(root, "text", mensaje);
    char *post_data = cJSON_PrintUnformatted(root);

    ESP_LOGW(TAG_HTTP, "Enviaré POST");
    esp_http_client_set_method(cliente_http, HTTP_METHOD_POST);
    esp_http_client_set_header(cliente_http, "Content-Type", "application/json");
    esp_http_client_set_post_field(cliente_http, post_data, strlen(post_data));

    mensaje_error = esp_http_client_perform(cliente_http);
    if (mensaje_error == ESP_OK) {
        ESP_LOGI(TAG_HTTP, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(cliente_http),
                esp_http_client_get_content_length(cliente_http));
        ESP_LOGW(TAG_HTTP, "Output del POST: %s", buffer_respuesta);

    } else
        ESP_LOGE(TAG_HTTP, "HTTP POST request failed: %s", esp_err_to_name(mensaje_error));

    memset(buffer_respuesta, NULL, MAX_HTTP_OUTPUT_BUFFER);
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

    http_app_start();
}
