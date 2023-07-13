/* Módulo: NVS (Non-Volatile Storage), partição da flash para salvar 
           dados e configurações do projeto.
*/

/* Includes */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <esp_task_wdt.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_rw.h"

/* Definições - debug */
#define NVS_TAG "NVS"

/* Definição - namespace */
#define NAMESPACE_NVS "cont"

/* Definições - semaforo */
#define TEMPO_PARA_OBTER_SEMAFORO_NVS (TickType_t)1

/* Variáveis estáticas */
static SemaphoreHandle_t semaforo_nvs;

/* Função: inicializa NVS
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void init_nvs(void)
{
    esp_err_t ret;

    ESP_LOGI(NVS_TAG, "Inicializando NVS...");
    ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    /* Inicialização do semáforo da NVS */
    semaforo_nvs = xSemaphoreCreateMutex();

    if (semaforo_nvs == NULL)
    {
        ESP_LOGE(NVS_TAG, "Erro ao configurar semaforo da NVS\n");
    }
    else
    {
        ESP_LOGI(NVS_TAG, "Semaforo da NVS configurado\n");
    }    

    ESP_LOGI(NVS_TAG, "Inicializacao da NVS completa");
}

/* Função: grava um valor de contador na NVS
 * Parâmetros: ponteiro para key do dado a ser salvo e valor a ser salvo
 * Retorno: ESP_OK: contador gravado com sucesso
 *          !ESP_OK: falha ao gravar contador
 */
esp_err_t grava_valor_contador_nvs(char *pt_key, uint32_t valor)
{
    esp_err_t ret = ESP_FAIL;
    nvs_handle handler_particao_nvs;

    if (xSemaphoreTake(semaforo_nvs, TEMPO_PARA_OBTER_SEMAFORO_NVS) != pdTRUE)
    {
        ESP_LOGE(NVS_TAG, "Erro: semaforo ocupado");
        goto FINALIZA_GRAVACAO;
    }

    if (pt_key == NULL)
    {
        ESP_LOGE(NVS_TAG, "Erro: ponteiro para key eh nulo");
        ret = ESP_FAIL;
        goto FINALIZA_GRAVACAO;
    }

    ret = nvs_open(NAMESPACE_NVS, NVS_READWRITE, &handler_particao_nvs);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Falha ao abrir particao NVS");
        goto FINALIZA_GRAVACAO;
    }

    ret = nvs_set_u32(handler_particao_nvs, pt_key, valor);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Falha ao salvar valor de contador na particao NVS");
        goto FINALIZA_GRAVACAO;
    }

    ret = nvs_commit(handler_particao_nvs);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Falha ao fazer commit na NVS");
        goto FINALIZA_GRAVACAO;
    }

    ESP_LOGI(NVS_TAG, "Valor de contador salvo com sucesso na particao NVS");

FINALIZA_GRAVACAO:
    xSemaphoreGive(semaforo_nvs);
    return ret;
}

/* Função: faz a leitura de um valor de contador da NVS
 * Parâmetros: - ponteiro para key do dado a ser lido
               - ponteiro para variável que armazenará o contador lido
 * Retorno: ESP_OK: contador lido com sucesso
 *          !ESP_OK: falha ao ler contador
*/
esp_err_t le_valor_contador_nvs(char *pt_key, uint32_t * pt_valor)
{
    esp_err_t ret = ESP_FAIL;
    nvs_handle handler_particao_nvs;

    if (xSemaphoreTake(semaforo_nvs, TEMPO_PARA_OBTER_SEMAFORO_NVS) != pdTRUE)
    {
        ESP_LOGE(NVS_TAG, "Erro: semaforo ocupado");
        goto FINALIZA_LEITURA;
    }

    if (pt_key == NULL)
    {
        ESP_LOGE(NVS_TAG, "Erro: ponteiro para key eh nulo");
        ret = ESP_FAIL;
        goto FINALIZA_LEITURA;
    }

    if (pt_valor == NULL)
    {
        ESP_LOGE(NVS_TAG, "Erro: ponteiro para variavel do contador a ser lido eh nulo");
        ret = ESP_FAIL;
        goto FINALIZA_LEITURA;
    }

    ret = nvs_open(NAMESPACE_NVS, NVS_READWRITE, &handler_particao_nvs);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Falha ao abrir particao NVS");
        goto FINALIZA_LEITURA;
    }

    ret = nvs_get_u32(handler_particao_nvs, pt_key, pt_valor);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Falha ao ler contador da NVS. Retornando valor default (zero)...");
        *pt_valor = 0;
        goto FINALIZA_LEITURA;
    }

    ESP_LOGI(NVS_TAG, "Contador lido com sucesso da particao NVS");

FINALIZA_LEITURA:
    xSemaphoreGive(semaforo_nvs);
    return ret;
}

/* Função: limpa a NVS (deleta todos os dados salvos nela)
 * Parâmetros: nenhum
 * Retorno: ESP_OK: dado lido com sucesso
 *          !ESP_OK: falha ao ler dado
 */
esp_err_t limpa_nvs(void)
{
    esp_err_t ret;

    ESP_LOGI(NVS_TAG, "Limpando NVS");

    ret = nvs_flash_erase();

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Erro ao limpar NVS");
    }
    else
    {
        ESP_LOGI(NVS_TAG, "NVS limpa com sucesso");
    }

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(NVS_TAG, "Fim da rotina de limpeza da NVS");

    return ret;
}