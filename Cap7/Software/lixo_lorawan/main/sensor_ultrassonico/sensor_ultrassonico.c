/* Módulo LoRaWAN */
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sensor_ultrassonico.h"

/* Biblioteca do HC-SR04 (https://github.com/UncleRus/esp-idf-lib/) */
#include <ultrasonic.h>

/* Tamanho do buffer do sensor ultrassônico e tempo entre leituras */
#define TAM_BUFFER_DISTANCIAS                   100
#define TEMPO_ENTRE_LEITURAS                    100 //ms

/* Tag de debug */
static const char* TAG_LOGS_SENSORES = "SENSORES";

/* Variáveis especificas do sensor ultrassônico */
ultrasonic_sensor_t sensor_ultrassonico;
float buffer_sensor_ultrassonico[TAM_BUFFER_DISTANCIAS] = {0.0};

/* Função: inicializa sensor
 * Parâmetros: ponteiro para estrutura de configuração do sensor
 * Retorno: nenhum 
 */
void inicializa_sensor(TConfig_sensores * pt_config_sensores)
{
    gpio_config_t io_conf_liga_desliga = {};    
    int cont_leituras = 0;
    float distancia_lida = 0.0;

    /* Configura liga/desliga do sensor */     
    io_conf_liga_desliga.intr_type = GPIO_INTR_DISABLE;  //Desabilita interrupção    
    io_conf_liga_desliga.mode = GPIO_MODE_OUTPUT;  //configura GPIO como saída
    io_conf_liga_desliga.pin_bit_mask = (1ULL<<pt_config_sensores->gpio_liga_desliga);  //máscara de bits relativa ao GPIO de saida
    io_conf_liga_desliga.pull_down_en = 0;  //desabilita pull-down
    io_conf_liga_desliga.pull_up_en = 0; //desabilita pull-up
    gpio_config(&io_conf_liga_desliga);

    /* Configura sensor HC-SR04 */     
    sensor_ultrassonico.trigger_pin = pt_config_sensores->gpio_trigger;
    sensor_ultrassonico.echo_pin = pt_config_sensores->gpio_echo;
    ultrasonic_init(&sensor_ultrassonico);

    /* Preenche buffer de distâncias */
    cont_leituras = 1;
    while (cont_leituras < TAM_BUFFER_DISTANCIAS)
    {
        esp_task_wdt_reset();

        if (ultrasonic_measure(&sensor_ultrassonico, MAX_DISTANCE_CM, &distancia_lida) != ESP_OK) 
        {
            ESP_LOGE(TAG_LOGS_SENSORES, "erro ao ler sensor ultrassonico. Tentando novamente em 1s ...");   
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {
            distancia_lida = distancia_lida*100.0;             
            buffer_sensor_ultrassonico[cont_leituras] = distancia_lida;
            ESP_LOGI(TAG_LOGS_SENSORES, "Leitura %d: %.2fcm", cont_leituras, buffer_sensor_ultrassonico[cont_leituras]);   
            cont_leituras++;
            vTaskDelay(pdMS_TO_TICKS(TEMPO_ENTRE_LEITURAS));
        }
    }
}

/* Função: le sensor de distância
 * Parâmetros: ponteiro para leitura do sensor
 * Retorno: nenhum 
 */
void le_sensor(TConfig_sensores * pt_config_sensores, float * pt_distancia_cm)
{
    float distancia_medida = 0.0;
    float soma_distancias = 0.0;
    int i = 0;

    /* Le HC-SR04 */
    gpio_set_level(pt_config_sensores->gpio_liga_desliga, 1);

    while (1)
    {
        esp_task_wdt_reset();
        
        if (ultrasonic_measure(&sensor_ultrassonico, MAX_DISTANCE_CM, &distancia_medida) != ESP_OK) 
        {
            ESP_LOGE(TAG_LOGS_SENSORES, "erro ao ler sensor ultrassonico. Tentando novamente em 1s ...");   
            vTaskDelay(pdMS_TO_TICKS(TEMPO_ENTRE_LEITURAS));
        }
        else
        {
            /* Aplica filtro de média móvel */
            for(i=0; i<(TAM_BUFFER_DISTANCIAS-1); i++)
            {
                buffer_sensor_ultrassonico[i] = buffer_sensor_ultrassonico[i+1];
            }

            distancia_medida = distancia_medida*100.0;
            buffer_sensor_ultrassonico[TAM_BUFFER_DISTANCIAS-1] = distancia_medida;

            soma_distancias = 0.0;
            for(i=0; i<TAM_BUFFER_DISTANCIAS; i++)
            {
                soma_distancias = soma_distancias + buffer_sensor_ultrassonico[i];
                ESP_LOGI(TAG_LOGS_SENSORES, "Valor: %.2f cm; Soma: %.2f cm", buffer_sensor_ultrassonico[i], soma_distancias);   
            }

            *pt_distancia_cm = soma_distancias/TAM_BUFFER_DISTANCIAS;

            break;
        }
    }

    gpio_set_level(pt_config_sensores->gpio_liga_desliga, 0);    
    ESP_LOGI(TAG_LOGS_SENSORES, "Sensor ultrassonico lido: distancia: %.2f cm\n", *pt_distancia_cm);
}