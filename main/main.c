#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#include "ds18b20.h"  // Librería para la sonda de temperatura.
#include "conexion_mqtt_thingsboard.c"
#include "conexion_http_telegram.c"
#include "credentials.h"


#define BOMBA_N_PIN         GPIO_NUM_25  // Bombas peristálticas, Nitrógeno.
#define BOMBA_P_PIN         GPIO_NUM_26  // Fósforo.
#define BOMBA_K_PIN         GPIO_NUM_27  // Potasio.
#define VALVULA_PIN_1       GPIO_NUM_18  // Válvula solenoide (2 pines).
#define VALVULA_PIN_2       GPIO_NUM_19
#define TEMP_SENSOR_PIN     GPIO_NUM_22  // Sonda de temperatura.
#define NIVEL_SENSOR_PIN    GPIO_NUM_23  // Sensor de nivel de líquidos.
#define ELECTRO_SENSOR_PIN  ADC1_CHANNEL_6  // Sensor de electroconductividad (GPIO 34).
 
static const char *SBC = PROYECTO;

static esp_adc_cal_characteristics_t adc1_chars;

// Constantes para calcular la electroconductividad.
#define EC_VALOR_K  1.0
#define EC_RES2     (7500.0/0.66)
#define EC_REF      20.0

float tempValor = 25.0, ecValor, ecVoltaje, ecAux;
bool nivelValor;

enum estado {CERRADA , ABIERTA};  // Estado para las bombas y la válvula.
enum estado valvula = CERRADA;  // Empiezan cerradas.
enum estado bombaN = CERRADA, bombaP = CERRADA, bombaK = CERRADA;

enum estado1 {MEDIR, DISPENSAR, VACIAR, TRANSMITIR, RECIBIR};
enum estado1 modo = MEDIR;  // Cotrolará el modo en el que trabaja el sistema.

enum estado2 {VACIO, LLENO};
enum estado2 tanque = VACIO;  // Estado del tanque dónde se dispensarán los nutrientes.


void medir_temperatura(void) {
    tempValor = ds18b20_get_temp();  // Obtenemos la temperatura del sensor.
    printf("Temperatura: %0.1f ºC\n", tempValor);
}

void medir_nivel_tanque(void) {
    nivelValor = !(bool) gpio_get_level(NIVEL_SENSOR_PIN);  // Hay que invertir el valor dado por el sensor.
    if (nivelValor) {
        tanque = LLENO;
        printf("Hay agua en el tanque\n");
    } else {
        tanque = VACIO;
        printf("No hay agua en el tanque\n");
    }
}

void medir_electroconductividad(void) {
    ecVoltaje = esp_adc_cal_raw_to_voltage(adc1_get_raw(ELECTRO_SENSOR_PIN), &adc1_chars);  // Voltaje del sensor de electroconductividad.
    ecAux = 1000*ecVoltaje/EC_RES2/EC_REF*EC_VALOR_K*10.0;
    ecValor = ecAux / (1.0+0.0185*(tempValor-25.0));  // Compensación de temperatura.
    printf("Electroconductividad: %0.1f ms/cm\n", ecValor);
}

void control_Bombas_Valvula(void) {
    while (1) {
        if (valvula == ABIERTA) {
            gpio_set_level(VALVULA_PIN_1, 1);
            gpio_set_level(VALVULA_PIN_2, 0);
        } else {  // CERRADA.
            gpio_set_level(VALVULA_PIN_1, 0);
            gpio_set_level(VALVULA_PIN_2, 1);
        }
        if (bombaN == ABIERTA)
            gpio_set_level(BOMBA_N_PIN, 1);
        else  // CERRADA.
            gpio_set_level(BOMBA_N_PIN, 0);
        if (bombaP == ABIERTA)
            gpio_set_level(BOMBA_P_PIN, 1);
        else  // CERRADA.
            gpio_set_level(BOMBA_P_PIN, 0);
        if (bombaK == ABIERTA)
            gpio_set_level(BOMBA_K_PIN, 1);
        else  // CERRADA.
            gpio_set_level(BOMBA_K_PIN, 0);
        
        vTaskDelay(50);
    }
}

void principal(void) {
    enum estado1 modo_ant = RECIBIR;  // Modo previo del sistema.
    enum estado1 modo_aux = modo;
    int nutrientes = 0;  // Controla el nutriente que se dispensa: 0=nada, 1=N, 2=P, 3=K.

    while (1) {
        switch (modo) {
            case MEDIR:  // Recabar datos de todos los sensores.
                medir_temperatura();
                medir_nivel_tanque();
                medir_electroconductividad();
                modo = modo_ant;
                break;

            case DISPENSAR:  // Dispensar nutrientes.
                // ¿Dispensar por pasos?
                if (nutrientes == 0) {  // Dispensar N.

                }
                else if (nutrientes == 1) {  // Dispensar P.

                }
                else if (nutrientes == 2) {  // Dispensar K.
                    
                }
                else {  // Ya se han dispensado todos los nutrientes. 
                    nutrientes = 0;
                    modo = TRANSMITIR;
                }
                break;

            case VACIAR:  // Vaciar tanque.
                if (tanque == LLENO) {
                    valvula = ABIERTA;
                    printf("Vaciando el tanque del agua.\n");
                    vTaskDelay(3000);
                    modo = MEDIR;
                }
                else {  // VACIO.
                    valvula = CERRADA;
                    modo = TRANSMITIR;
                }
                break;

            case TRANSMITIR:  // Transmitir datos a ThingsBoard.
                if (modo_ant == DISPENSAR) {
                    mqtt_mandar_datos(tempValor, nivelValor, ecValor);
                    modo = DISPENSAR;
                } 
                else if (modo_ant == MEDIR || modo_ant == VACIAR) {
                    mqtt_mandar_datos(tempValor, nivelValor, ecValor);
                    modo = RECIBIR;
                } 
                else  // RECIBIR.
                    modo = MEDIR;        
                break;
            
            case RECIBIR:  // Recibir datos de Telegram.
                // Dependiendo del lo que reciba irá a DISPENSAR o a TRANSMITIR.
                break;
            
            default:
                break;
        }
        modo_ant = modo_aux;
        modo_aux = modo;

        vTaskDelay(100);
    }
}

void app_main(void) {
    // Configuramos los puertos GPIO y el ADC de los dispositivos.
    gpio_set_direction(BOMBA_N_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOMBA_P_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOMBA_K_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(VALVULA_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(VALVULA_PIN_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(TEMP_SENSOR_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(NIVEL_SENSOR_PIN, GPIO_MODE_INPUT);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ELECTRO_SENSOR_PIN, ADC_ATTEN_DB_11));

    ds18b20_init(TEMP_SENSOR_PIN);  // Iniciamos la sonda de temperatura.
    iniciar_mqtt();  // Iniciamos la conexión MQTT.
    //iniciar_http();  // Iniciamos la conexión HTTP.
    
    mqtt_mandar_credenciales_telegram();  // Mandamos las credenciales de Telegram a ThingsBoard como atributos del servidor.

    // Iniciamos las tareas.
    xTaskCreate(principal, "Modos sistema", 512, NULL, 9, NULL);
    xTaskCreate(control_Bombas_Valvula, "Abrir_Cerrar", 512, NULL, 8, NULL);

    float i = 0.5;
    tempValor = 19.5;

    while(1){     
        //medir_temperatura();
        medir_nivel_tanque();
        medir_electroconductividad();
        //nivelValor = 0;
        tempValor += i;
        mqtt_mandar_datos(tempValor, nivelValor, ecValor);
        vTaskDelay(1500);
        if (tempValor > 27.5)
            i = -0.5;
        else if (tempValor < 17.5)
            i = 0.5;
    }
}
