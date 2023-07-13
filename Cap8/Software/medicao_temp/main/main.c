#include <stdio.h>
#include <esp_task_wdt.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

/* Includes de outros módulos */
#include "LoRaWAN/LoRaWAN.h"
#include "medicao_temperatura/medicao_temperatura.h"

/* Includes dos header files com as priorizações e tamanho das stacks das tarefas */
#include "prio_tasks.h"
#include "stacks_sizes.h"

/* Definições - tarefa de envios LoRaWAN */
#define PARAMETROS_TASK_MEDICAO_TEMP     NULL
#define HANDLER_TASK_MEDICAO_TEMP        NULL

/* Definição - tempo máximo sem feed do watchdog */
#define TEMPO_MAX_SEM_FEED_WATCHDOG        60 //s

/* Definição - tag para debug */
#define MAIN_TAG    "MAIN"

/* Definições - temperaturas para envio LoRaWAN */
#define TAM_ARRAY_TEMP_ENVIO   4
#define IDX_TEMP_MEDIA         0
#define IDX_TEMP_MINIMA        1
#define IDX_TEMP_MAXIMA        2
#define IDX_DESVIO_PADRAO_X10  3

/* Variável para indicar se está durante o tempo de burn-in para o sensor de temperatura*/
static bool esta_em_tempo_de_burn_in = true;

/* Tarefa do projeto */
static void faz_medicao_temp(void *arg);

/* Protótipos */
static unsigned long diferenca_tempo(unsigned long tref);

/* Função: calcula diferença de tempo do instante atual e uma referência de tempo
 *  Parâmetros: referência de tempo
 *  Retorno: diferença calculada
*/
static unsigned long diferenca_tempo(unsigned long tref)
{
    int64_t timestamp_atual = 0;

    timestamp_atual = esp_timer_get_time() / 1000;
    return (timestamp_atual - tref);
}

/* Função: tarefa de medição de temperatura, cálculo do desvio padrão
 *         e envio para nuvem via LoRaWAN
 * Parâmetros: argumentos da tarefa
 * Retorno: nenhum
 */
static void faz_medicao_temp(void *arg)
{
    int64_t timestamp_medicao_temperatura = 0;
    int64_t timestamp_envio_temperatura = 0;
    int64_t timestamp_burn_in_sensor_temp = 0;
    int8_t array_temperaturas_envio[TAM_ARRAY_TEMP_ENVIO] = {0};

    /* Habilita o watchdog para esta tarefa */
    esp_task_wdt_add(NULL);

    ESP_LOGI(MAIN_TAG, "Programa iniciado. Entrando em fase de espera pelo tempo de burn-in do sensor de temperatura...");
    
    /* Inicializa temporizações */ 
    timestamp_medicao_temperatura = esp_timer_get_time() / 1000;
    timestamp_envio_temperatura = esp_timer_get_time() / 1000;
    timestamp_burn_in_sensor_temp = esp_timer_get_time() / 1000;

    while(1)
    {
        /* Alimenta o watchdog */
        esp_task_wdt_reset();

        /* Enquanto estiver em tempo de burn-in, nenhuma medição deve acontecer */
        if ( (diferenca_tempo(timestamp_burn_in_sensor_temp) >= TEMPO_BURN_IN_SENSOR_TEMP) && (esta_em_tempo_de_burn_in == true) )
        {
            /* Após o tempo de burn-in do sensor de temperartura, as medições de 
             * temperaturas e posteriores envios estão liberados
             */
            timestamp_envio_temperatura = esp_timer_get_time() / 1000;
            timestamp_medicao_temperatura = esp_timer_get_time() / 1000;
            esta_em_tempo_de_burn_in = false;
            ESP_LOGI(MAIN_TAG, "Fase de burn-in do sensor de temperatura terminou.");
        }

        /* Verifica se é o momento de fazer uma nova medição de temperatura.
         * Se sim, faz a leitura e inserção da mesma no buffer de medições.
         *
         * OBS: a medição só é feita se o tempo de burn-in já passou.
         */
        if ( (esta_em_tempo_de_burn_in == false) &&
             (diferenca_tempo(timestamp_medicao_temperatura) >= TEMPO_ENTRE_LEITURAS_SUCESSIVAS_TEMPERATURA) )
        {
            le_temperatura_atual_e_insere_buffer();

            /* Reinicia temporização da medição de temperaturas */
            timestamp_medicao_temperatura = esp_timer_get_time() / 1000;
        }

        /* Verifica se é o momento de fazer um envio de temperaturas (média, mínima e máxima),
         * assim como o desvio padrão (x10).
         * O envio só é feito quando o buffer de amostras de temperaturas está cheio.
         * 
         * OBS: o envio só é feito se o tempo de burn-in já passou.
         */
        if ( (esta_em_tempo_de_burn_in == false) &&
             (diferenca_tempo(timestamp_envio_temperatura) >= TEMPO_ENTRE_TRANSMISSOES) &&
             (quantidade_de_temperaturas_lidas() == QTDE_AMOSTRAS_TEMPERATURA) )
        {
            /* Obtém temperaturas média, máxima e mínima, assim como o 
             * desvio padrão (x10) das amostras de temperatura 
             */
            array_temperaturas_envio[IDX_TEMP_MEDIA] = obtem_media_temperaturas();
            array_temperaturas_envio[IDX_TEMP_MAXIMA] = obtem_temperatura_maxima();
            array_temperaturas_envio[IDX_TEMP_MINIMA] = obtem_temperatura_minima();
            array_temperaturas_envio[IDX_DESVIO_PADRAO_X10] = calcula_desvio_padrao_x10();
            ESP_LOGI(MAIN_TAG, "Resumo:");
            ESP_LOGI(MAIN_TAG, "- Quantidade de temperaturas: %d", QTDE_AMOSTRAS_TEMPERATURA);
            ESP_LOGI(MAIN_TAG, "- Temperatura media: %dC", array_temperaturas_envio[IDX_TEMP_MEDIA]);
            ESP_LOGI(MAIN_TAG, "- Temperatura minima: %dC", array_temperaturas_envio[IDX_TEMP_MINIMA]);
            ESP_LOGI(MAIN_TAG, "- Temperatura maxima: %dC", array_temperaturas_envio[IDX_TEMP_MAXIMA]);
            ESP_LOGI(MAIN_TAG, "- Desvio padrao das temperaturas (x10): %dC", array_temperaturas_envio[IDX_DESVIO_PADRAO_X10]);

            /* Faz envio das temperaturas por LoRaWAN */
            envia_mensagem_binaria_lorawan_ABP((char *)array_temperaturas_envio, TAM_ARRAY_TEMP_ENVIO);

            /* Reinicializa medições medições de temperatura, limpando buffer de amostras
             * de temperaturas 
             */
            reinicializa_medicoes_temperatura();

            /* Reinicia temporização do envio de temperaturas */
            timestamp_envio_temperatura = esp_timer_get_time() / 1000;
        }

        /* Aguarda 10ms para verificar novamente as temporizações */
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
   esp_task_wdt_init(TEMPO_MAX_SEM_FEED_WATCHDOG, true);

   /* Inicializa medição de temperatura */
   ESP_LOGI(MAIN_TAG, "Inicializando medicao de temperatura...");
   esta_em_tempo_de_burn_in = true;
   init_medicao_temperatura();
   ESP_LOGI(MAIN_TAG, "Medicao de temperatura inicializada");

   /* Inicializa LoRaWAN */
   ESP_LOGI(MAIN_TAG, "Inicializando LoRaWAN...");
   init_lorawan();
   ESP_LOGI(MAIN_TAG, "LoRaWAN inicializado");

   /* Criação /agendamento da tarefa do projeto, responsável por:
    * - Fazer medições de temperatura
    * - Calcular o desvio padrão das medições
    * - Enviar temperatura média e desvio padrão para a nuvem,
    *   via LoRaWAN.
    */
   xTaskCreate(faz_medicao_temp, "MEDICAO_TEMP", 
                                 MEDICAO_TEMP_TAM_TASK_STACK,
                                 PARAMETROS_TASK_MEDICAO_TEMP,
                                 PRIO_TASK_MEDICAO_TEMP,
                                 HANDLER_TASK_MEDICAO_TEMP);
}
