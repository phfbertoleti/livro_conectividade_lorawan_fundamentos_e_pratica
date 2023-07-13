/* Módulo: envios LoRaWAN */

/* Includes */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "contadores_de_pulsos.h"

/* Includes de outros modulos */
#include "../nvs_rw/nvs_rw.h"

/* Definição - debug */
#define CONTADORES_PULSOS_TAG "CONTADORES_PULSOS"

/* Definições - inserção e leitura de dados */
#define TEMPO_MAX_PARA_INSERIR_DADO_FILA    ( TickType_t ) 1
#define TEMPO_MAX_PARA_LER_DADO_FILA        ( TickType_t ) 100

/* Definição - flag de interrupção externa */
#define ESP_INTR_FLAG_DEFAULT    0

/* Definição - tempo para debounce */
#define TEMPO_DEBOUNCE_PULSOS          200  //ms

/* Definições - PIN SEL para os GPIOs que receberão os pulsos */
#define GPIO_INPUT_CONTADOR_1_PIN_SEL  (1ULL<<GPIO_CONTADOR_1)
#define GPIO_INPUT_CONTADOR_2_PIN_SEL  (1ULL<<GPIO_CONTADOR_2)

/* Variaveis dos contadores de pulsos */
static uint32_t contador_pulsos_1 = 0;
static uint32_t contador_pulsos_2 = 0;
static int64_t tempo_ref_contador_1 = 0;
static int64_t tempo_ref_contador_2 = 0;

/*
 *  Handlers das ISR dos contadores de pulsos
 */
static void IRAM_ATTR contador_1_isr_handler(void* arg)
{
    int64_t tempo_atual = esp_timer_get_time() / 1000;

    /* Tempo debounce: 100ms */
    if ( (tempo_atual - tempo_ref_contador_1) >= TEMPO_DEBOUNCE_PULSOS)
    {
        contador_pulsos_1++;
        xQueueOverwriteFromISR(fila_contador_pulsos_1, &contador_pulsos_1, NULL);
        tempo_ref_contador_1 = esp_timer_get_time() / 1000;
    }
}

static void IRAM_ATTR contador_2_isr_handler(void* arg)
{
    int64_t tempo_atual = esp_timer_get_time() / 1000;

    /* Tempo debounce: 100ms */
    if ( (tempo_atual - tempo_ref_contador_2) >= TEMPO_DEBOUNCE_PULSOS)
    {
        contador_pulsos_2++;
        xQueueOverwriteFromISR(fila_contador_pulsos_2, &contador_pulsos_2, NULL);
        tempo_ref_contador_2 = esp_timer_get_time() / 1000;
    }
}

/* Função: inicializa contadores de pulsos
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void init_contadores_de_pulsos(void)
{
    gpio_config_t io_conf_contadores = {};

    ESP_LOGI(CONTADORES_PULSOS_TAG, "Inicializando contadores de pulsos...");

    /* Cria / aloca fila de bytes recebidos */
    fila_contador_pulsos_1 = xQueueCreate(1, sizeof(uint32_t));
    fila_contador_pulsos_2 = xQueueCreate(1, sizeof(uint32_t));

    /* Cria / aloca filas e as inicializa */
    if ((fila_contador_pulsos_1 == NULL) || (fila_contador_pulsos_2 == NULL) )
    {
        ESP_LOGE(CONTADORES_PULSOS_TAG, "Falha ao criar/alocar filas. ESP32 sera reiniciado");
        esp_restart();        
    }
    else
    {
        ESP_LOGI(CONTADORES_PULSOS_TAG, "Filas criadas / alocadas com sucesso");
        le_valor_contador_nvs(CHAVE_NVS_CONTADOR_1, &contador_pulsos_1);
        le_valor_contador_nvs(CHAVE_NVS_CONTADOR_2, &contador_pulsos_2);
        while (xQueueOverwrite(fila_contador_pulsos_1, (void *)&contador_pulsos_1) != pdPASS);
        while (xQueueOverwrite(fila_contador_pulsos_2, (void *)&contador_pulsos_2) != pdPASS);
    }

    /* Configura GPIOs que receberão os pulsos */
    /* Contadores: input, com pull-up interno e interrupção na borda de descida */
    io_conf_contadores.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_contadores.pin_bit_mask = GPIO_INPUT_CONTADOR_1_PIN_SEL;
    io_conf_contadores.mode = GPIO_MODE_INPUT;
    io_conf_contadores.pull_up_en = 1;
    gpio_config(&io_conf_contadores);

    io_conf_contadores.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_contadores.pin_bit_mask = GPIO_INPUT_CONTADOR_2_PIN_SEL;
    io_conf_contadores.mode = GPIO_MODE_INPUT;
    io_conf_contadores.pull_up_en = 1;
    gpio_config(&io_conf_contadores);

    //Instala ISR dos GPIOs
    ESP_LOGI(CONTADORES_PULSOS_TAG, "Instalando ISRs dos contadores");
    tempo_ref_contador_1 = esp_timer_get_time() / 1000;
    tempo_ref_contador_2 = esp_timer_get_time() / 1000;
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_CONTADOR_1, contador_1_isr_handler, (void*) GPIO_CONTADOR_1);
    gpio_isr_handler_add(GPIO_CONTADOR_2, contador_2_isr_handler, (void*) GPIO_CONTADOR_2);
    
    ESP_LOGI(CONTADORES_PULSOS_TAG, "Contadores de pulsos inicializados");
}