/* Módulo: envios LoRaWAN */

/* Includes */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/queue.h"

/* Includes dos módulos do software */
#include "../LoRaWAN/LoRaWAN.h"
#include "../contadores_de_pulsos/contadores_de_pulsos.h"
#include "../nvs_rw/nvs_rw.h"

/* Includes de parametrização das tarefas */
#include "../prio_tasks.h"
#include "../stacks_sizes.h"

/* Definição - debug */
#define ENVIOS_LORAWAN_TAG "ENVIOS_LORAWAN"

/* Definição - tempo minimo entre envios */
#define TEMPO_MIN_ENTRE_ENVIOS_LORAWAN_MS   15000 //ms

/* Definições - inserção e leitura de dados */
#define TEMPO_MAX_PARA_INSERIR_DADO_FILA    ( TickType_t ) 1
#define TEMPO_MAX_PARA_LER_DADO_FILA        ( TickType_t ) 100

/* Definições - tarefa de envios LoRaWAN */
#define PARAMETROS_TASK_ENVIOS_LORAWAN NULL
#define HANDLER_TASK_ENVIOS_LORAWAN NULL
#define CPU_TASK_ENVIOS_LORAWAN 0

/* Variáveis locais */
static uint32_t total_de_envios = 0;

/* Tarefas deste módulo */
static void envios_lorawan_task(void *arg);

/* Função: inicializa envios LoRaWAN
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void init_envios_lorawan(void)
{
    ESP_LOGI(ENVIOS_LORAWAN_TAG, "Inicializando envios LoRaWAN...");

    /* Inicializa totalizador de envios LoRaWAN */
    total_de_envios = 0;

    /* Inicializa a tarefa que gerencia os comandos */
    xTaskCreatePinnedToCore(envios_lorawan_task, "envios_lorawan",
                            ENVIOS_LORAWAN_TAM_TASK_STACK,
                            PARAMETROS_TASK_ENVIOS_LORAWAN,
                            PRIO_TASK_ENVIOS_LORAWAN,
                            HANDLER_TASK_ENVIOS_LORAWAN,
                            CPU_TASK_ENVIOS_LORAWAN);

    ESP_LOGI(ENVIOS_LORAWAN_TAG, "Envios LoRaWAN inicializados");
}

/* Função: tarefa para envio LoRaWAN
 * Parâmetros: argumentos da task
 * Retorno: nenhum
 */
static void envios_lorawan_task(void *arg)
{
    char bytes_para_enviar[8] = {0};
    uint32_t contador_1 = 0;
    uint32_t contador_2 = 0;
    int qtde_bytes = 0;
    int i;
    char * pt_byte_contador;
    int64_t tempo_atual = 0;
    int64_t tempo_ref = 0;

    esp_task_wdt_add(NULL);

    tempo_ref = esp_timer_get_time() / 1000;

    while (1)
    {        
        /* Aguarda momento do envio */
        tempo_atual = esp_timer_get_time() / 1000;

        if ( (tempo_atual - tempo_ref) < TEMPO_MIN_ENTRE_ENVIOS_LORAWAN_MS )
        {
            esp_task_wdt_reset();
            continue;
        }
        else
        {
            tempo_ref = esp_timer_get_time() / 1000;
        }
        
        /* Le contadores de pulsos */
        qtde_bytes = 8;
        while (xQueuePeek(fila_contador_pulsos_1, &contador_1, TEMPO_MAX_PARA_LER_DADO_FILA) != pdPASS)
        {
            esp_task_wdt_reset();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        while (xQueuePeek(fila_contador_pulsos_2, &contador_2, TEMPO_MAX_PARA_LER_DADO_FILA) != pdPASS)
        {
            esp_task_wdt_reset();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        
        pt_byte_contador = (char *)&contador_1;
        bytes_para_enviar[0] = *pt_byte_contador;
        pt_byte_contador++;
        bytes_para_enviar[1] = *pt_byte_contador;
        pt_byte_contador++;
        bytes_para_enviar[2] = *pt_byte_contador;
        pt_byte_contador++;
        bytes_para_enviar[3] = *pt_byte_contador;

        pt_byte_contador = (char *)&contador_2;
        bytes_para_enviar[4] = *pt_byte_contador;
        pt_byte_contador++;
        bytes_para_enviar[5] = *pt_byte_contador;
        pt_byte_contador++;
        bytes_para_enviar[6] = *pt_byte_contador;
        pt_byte_contador++;
        bytes_para_enviar[7] = *pt_byte_contador;

        ESP_LOGI(ENVIOS_LORAWAN_TAG, "Payload a ser enviado:");
        for(i=0; i<8; i++)
        {
            ESP_LOGI(ENVIOS_LORAWAN_TAG, "Byte %d: %02X", i, bytes_para_enviar[i]);
        }

        envia_mensagem_binaria_lorawan_ABP(bytes_para_enviar, qtde_bytes);
        total_de_envios++;
        ESP_LOGI(ENVIOS_LORAWAN_TAG, "Envio #%d LoRaWAN feito. Envios faltantes para o salvamento na NVS: %d", total_de_envios,
                                                                                                               NUM_ENVIOS_PARA_GRAVAR_CONTADORES_NVS - total_de_envios);

        /* Verifica se é momento de salvar na NVS os valores dos contadores */
        if (total_de_envios == NUM_ENVIOS_PARA_GRAVAR_CONTADORES_NVS)
        {
            grava_valor_contador_nvs(CHAVE_NVS_CONTADOR_1, contador_1);
            grava_valor_contador_nvs(CHAVE_NVS_CONTADOR_2, contador_2);
            total_de_envios = 0;
        }

        /* Aguarda 10ms para reiniciar o ciclo */
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
