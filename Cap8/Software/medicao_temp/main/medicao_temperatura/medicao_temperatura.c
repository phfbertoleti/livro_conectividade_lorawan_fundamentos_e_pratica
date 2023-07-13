/* Módulo: LoRaWAN */

/* Includes */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <ds18x20.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_err.h"
#include "medicao_temperatura.h"

/* Includes - demais módulos */
#include "../LoRaWAN/LoRaWAN.h"

/* Definição - tag de debug */
#define MEDICAO_TEMP_TAG   "MEDICAO_TEMP"

/* Variáveis estáticas */
static int8_t amostras_temperatura[QTDE_AMOSTRAS_TEMPERATURA] = {0};
static int idx_temperatura = 0;

/* Coloque o endereço do seu sensor DS18B20 aqui. Para obtê-lo, observe as 
 * mensagens de debug do programa na sua inicialização (no ESP32 Monitor),
 * pois em toda inicialização é feito um scan de sensores ds18b20 e os 
 * endereços encontrados são escritos lá.
 */
static const ds18x20_addr_t endereco_sensor_ds18b20 = 0xc83ce10457027828;

/* Definição do GPIO usado para ler o sensor de temperarura */
static const gpio_num_t ds18b20_gpio = GPIO_SENSOR_DS18B20;

/* Funções locais */
static size_t faz_scan_sensores_ds18b20(void);

/* Função: inicializa medição de temperatura
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void init_medicao_temperatura(void)
{
    /* Inicializa variáveis */
    reinicializa_medicoes_temperatura(); 

    if (faz_scan_sensores_ds18b20() != 1)
    {
        /* Se não for possível encontrar o sensor, para o programa */
        ESP_LOGE(MEDICAO_TEMP_TAG, "Erro ao scanear o sensor DS18B20. O programa nao pode continuar.");
        while(1);
    }
}

/* Função: reinicializa medições de temperatura, limpando buffer de amostras
 *         de temperaturas
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void reinicializa_medicoes_temperatura(void)
{
    memset((char *)&amostras_temperatura, 0x00, sizeof(amostras_temperatura));
    idx_temperatura = 0;  
}

/* Função: faz scan pelos sensores
 * Parâmetros: nenhum
 * Retorno: numero de sensores encontrados
 */
static size_t faz_scan_sensores_ds18b20(void)
{
    esp_err_t status_scan_sensores;
    ds18x20_addr_t addrs[QTDE_MAX_SENSORES_DS18B20];
    size_t sensores_encontrados = 0;

    status_scan_sensores = ds18x20_scan_devices(ds18b20_gpio, 
                                                addrs,
                                                QTDE_MAX_SENSORES_DS18B20,
                                                &sensores_encontrados);

    if (status_scan_sensores != ESP_OK)
    {
        ESP_LOGE(MEDICAO_TEMP_TAG, "Erro ao fazer scan de sensores DS18b20: %d (%s)", status_scan_sensores, esp_err_to_name(status_scan_sensores));     
        sensores_encontrados = 0;   
        goto FIM_SCAN;
    }

    if (sensores_encontrados == 0)
    {
       ESP_LOGE(MEDICAO_TEMP_TAG, "Nenhum sensor DS18B20 encontrado!");
       goto FIM_SCAN;
    }

    ESP_LOGI(MEDICAO_TEMP_TAG, "%d sensor(es) DS18B20 detectado(s)", sensores_encontrados);
    ESP_LOGI(MEDICAO_TEMP_TAG, "Endereco do sensor 1: 0x%08" PRIx32 "%08" PRIx32,
                               (uint32_t)(addrs[0] >> 32), 
                               (uint32_t)addrs[0]);

FIM_SCAN:
    return sensores_encontrados;
}


/* Função: le temperatura atual e a insere no buffer de amostras
 *         de temperatura
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void le_temperatura_atual_e_insere_buffer(void)
{
    float temperatura_lida_float = 0.0;
    esp_err_t status_leitura_temperatura;

    if (idx_temperatura < QTDE_AMOSTRAS_TEMPERATURA)
    {
        /* Há espaço no buffer de temperaturas.
         * Faz medição e a insere no buffer.
         */
        status_leitura_temperatura = ds18x20_measure_and_read(ds18b20_gpio,
                                                              endereco_sensor_ds18b20,
                                                              &temperatura_lida_float);

        if (status_leitura_temperatura == ESP_OK)
        {
            /* Leitura bem sucedida */            
            amostras_temperatura[idx_temperatura] = (int8_t)temperatura_lida_float;
            ESP_LOGI(MEDICAO_TEMP_TAG, "Temperatura #%d/%d lida = %dC", idx_temperatura+1, QTDE_AMOSTRAS_TEMPERATURA, amostras_temperatura[idx_temperatura]);
            idx_temperatura++;            
        }
        else
        {
            /* Houve algum erro na leitura. Descarta a leitura feita */
            ESP_LOGE(MEDICAO_TEMP_TAG, "Ocorreu um erro na leitura de temperatura. A medicao atual esta descartada.");
        }
    }
}

/* Função: calcula desvio padrão (multiplicado por 10)
 * Parâmetros: nenhum
 * Retorno: desvio padrão (multiplicado por 10)
*/
int8_t calcula_desvio_padrao_x10(void)
{
    int8_t desvio_padrao_x10_calculado_int = 0;
    float desvio_padrao_x10_calculado_float = 0.0;
    float media_temperaturas = 0.0;
    float fator_variancia = 0.0;
    int i = 0;

    /* Calcula média das temperaturas */
    media_temperaturas = 0.0;
    for (i = 0; i < QTDE_AMOSTRAS_TEMPERATURA; i++)
    {
        media_temperaturas = media_temperaturas + amostras_temperatura[i];
    }

    media_temperaturas = media_temperaturas / QTDE_AMOSTRAS_TEMPERATURA;

    /* Calcula variância amostral */
    desvio_padrao_x10_calculado_float = 0.0;
    for (i = 0; i < QTDE_AMOSTRAS_TEMPERATURA; i++)
    {
        fator_variancia = (float)amostras_temperatura[i] - media_temperaturas;
        fator_variancia = fator_variancia * fator_variancia;
        desvio_padrao_x10_calculado_float = desvio_padrao_x10_calculado_float + fator_variancia;
    }

    desvio_padrao_x10_calculado_float = sqrt(desvio_padrao_x10_calculado_float / QTDE_AMOSTRAS_TEMPERATURA);
    desvio_padrao_x10_calculado_float = desvio_padrao_x10_calculado_float * 10.0;
    desvio_padrao_x10_calculado_int = (int8_t)desvio_padrao_x10_calculado_float;
    
    return desvio_padrao_x10_calculado_int;
}

/* Função: obtem temperatura máxima do array de 
 *         amopstras de temperaturas
 *  Parâmetros: nenhum
 *  Retorno: temperatura máxima
*/
int8_t obtem_temperatura_maxima(void)
{
    int8_t temp_max = -10;
    int i;

    for (i = 0; i < QTDE_AMOSTRAS_TEMPERATURA; i++)
    {
        if ( amostras_temperatura[i] > temp_max)
        {
            temp_max = amostras_temperatura[i];
        }
    }

    return temp_max;
}

/* Função: obtem temperatura mínima do array de amostras
 *         de temperaturas
 *  Parâmetros: nenhum
 *  Retorno: temperatura mínima
*/
int8_t obtem_temperatura_minima(void)
{
    int8_t temp_min = 110;
    int i;

    for (i = 0; i < QTDE_AMOSTRAS_TEMPERATURA; i++)
    {
        if ( amostras_temperatura[i] < temp_min)
        {
            temp_min = amostras_temperatura[i];
        }
    }

    return temp_min;
}

/* Função: obtem média das temperaturas até o momento
 *  Parâmetros: nenhum
 *  Retorno: média calculada
*/
int8_t obtem_media_temperaturas(void)
{
    int i;
    int soma_temp = 0;
    int8_t media = 0;

    for (i = 0; i < QTDE_AMOSTRAS_TEMPERATURA; i++)
    {
        soma_temp = soma_temp + amostras_temperatura[i];
    }

    media = (int8_t)(soma_temp / QTDE_AMOSTRAS_TEMPERATURA);
    return media;
}

/* Função: retorna a quantidade de temperaturas lidas ate
 *         o momento
 *  Parâmetros: nenhum
 *  Retorno: quantidade de temperaturas lidas ate o momento
*/
int quantidade_de_temperaturas_lidas(void)
{
    return idx_temperatura;
}