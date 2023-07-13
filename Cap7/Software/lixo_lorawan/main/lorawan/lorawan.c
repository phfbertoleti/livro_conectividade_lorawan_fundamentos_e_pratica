/* Módulo LoRaWAN */
#include <string.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "lorawan.h"

/* Definições da UART de comunicação com módulo LoRaWAN */
#define SENS_LORAWAN_TEST_TXD (CONFIG_SENSORES_LORAWAN_UART_TXD)
#define SENS_LORAWAN_TEST_RXD (CONFIG_SENSORES_LORAWAN_UART_RXD)
#define SENS_LORAWAN_TEST_RTS (UART_PIN_NO_CHANGE)
#define SENS_LORAWAN_TEST_CTS (UART_PIN_NO_CHANGE)
#define SENS_LORAWAN_UART_PORT_NUM (CONFIG_SENSORES_LORAWAN_UART_PORT_NUM)
#define SENS_LORAWAN_UART_BAUD_RATE (CONFIG_SENSORES_LORAWAN_UART_BAUD_RATE)

/* Tamanho do buffer da UART */
#define BUF_SIZE (512)

/* Buffer de recepção UART */
static char buffer_recepcao[BUF_SIZE] = {0};

/* Tag de debug */
static const char *TAG_LOGS_LORAWAN = "LORAWAN";

/* Função: envia comando AT via UART para módulo LoRaWAN
 * Parâmetros: estrutura de configuração LoRaWAN
 * Retorno: nenhum
 */
void envia_comando_uart(char *pt_cmd, int tamanho)
{
    int tam = 0;
    bool houve_resposta_busy = false;

    do
    {
        uart_write_bytes(SENS_LORAWAN_UART_PORT_NUM, pt_cmd, tamanho);
        vTaskDelay(pdMS_TO_TICKS(TEMPO_ENTRE_COMANDOS_AT));
        esp_task_wdt_reset();

        memset(buffer_recepcao, 0x00, BUF_SIZE);
        tam = uart_read_bytes(SENS_LORAWAN_UART_PORT_NUM,
                              (char *)buffer_recepcao, (BUF_SIZE - 1),
                              TEMPO_ENTRE_COMANDOS_AT / portTICK_PERIOD_MS);

        /* Verifica se não houve resposta de busy */
        if (strstr(buffer_recepcao, "BUSY") != NULL)
        {
            /* Busy detectado. O comando deve ser enviado novamente. */
            ESP_LOGE(TAG_LOGS_LORAWAN, "BUSY detectado na resposta: %s. Reenviando comando em 5 segundos...", buffer_recepcao);
            vTaskDelay(pdMS_TO_TICKS(5000));
            esp_task_wdt_reset();
            houve_resposta_busy = true;
        }
        else
        {
            ESP_LOGI(TAG_LOGS_LORAWAN, "BUSY nao detectado na resposta. Prosseguindo com o software");
            houve_resposta_busy = false;
        }
    } while (houve_resposta_busy == true);

    if (tam > 0)
    {
        ESP_LOGI(TAG_LOGS_LORAWAN, "Dados recebidos: %s", buffer_recepcao);
    }
    else
    {
        ESP_LOGI(TAG_LOGS_LORAWAN, "Nenhum dado recebido");
    }
}

/* Função: inicializa UART de comunicação com módulo LoRaWAN
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void inicializa_uart_lorawan(void)
{
    /* Configura UART */
    uart_config_t uart_config = {
        .baud_rate = SENS_LORAWAN_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(SENS_LORAWAN_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(SENS_LORAWAN_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(SENS_LORAWAN_UART_PORT_NUM, SENS_LORAWAN_TEST_TXD, SENS_LORAWAN_TEST_RXD, SENS_LORAWAN_TEST_RTS, SENS_LORAWAN_TEST_CTS));
}

/* Função: configura módulo LoRaWAN segundo estrutura de configuração LoRaWAN
 * Parâmetros: estrutura de configuração LoRaWAN
 * Retorno: nenhum
 */
void configurar_lorawan(TConfig_LoRaWAN *pt_lorawan)
{
    char cmd_at[TAM_MAX_CMD_AT] = {0};

    esp_task_wdt_reset();

    /* Configuração da máscara de canais (para LA915) */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao da mascara de canais em %s ...", pt_lorawan->CHMASK);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+CHMASK=%s\n\r", pt_lorawan->CHMASK);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração do join mode para ABP */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao do join mode ...");
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+NJM=%c\n\r", pt_lorawan->join_mode);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração do endereço LoRaWAN */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao do endereco LoRaWAN em %s ...", pt_lorawan->DEVADDR);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+DADDR=%s\n\r", pt_lorawan->DEVADDR);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração do Application EUI */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao do Application EUI em %s ...", pt_lorawan->APPEUI);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+APPEUI=%s\n\r", pt_lorawan->APPEUI);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração do Application Session Key */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao do Application Session Key em %s ...", pt_lorawan->APPSKEY);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+APPSKEY=%s\n\r", pt_lorawan->APPSKEY);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração do Network Session Key */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao do Network Session Key em %s ...", pt_lorawan->NWSKEY);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+NWKSKEY=%s\n\r", pt_lorawan->NWSKEY);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração do ADR como desabilitado */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao do ADR em %c ...", pt_lorawan->adr);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+ADR=%c\n\r", pt_lorawan->adr);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração do Data rate para menor payload e maior alcance possiveis */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao do DR como %c ...", pt_lorawan->dr);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+DR=%c\n\r", pt_lorawan->dr);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração classe do dispositivo LoRaWAN como A */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao da classe do dispositivo como %c ...", pt_lorawan->classe);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+CLASS=%c\n\r", pt_lorawan->classe);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    /* Configuração da confirmação de envio como ligado */
    ESP_LOGI(TAG_LOGS_LORAWAN, "Configuracao da confirmacao de envio em %c ...", pt_lorawan->confirmacao_de_envio);
    memset(cmd_at, 0x00, sizeof(cmd_at));
    snprintf(cmd_at, sizeof(cmd_at), "AT+CFM=%c\n\r", pt_lorawan->confirmacao_de_envio);
    envia_comando_uart(cmd_at, strlen(cmd_at));
    esp_task_wdt_reset();

    ESP_LOGI(TAG_LOGS_LORAWAN, "Modulo LoRaWAN totalmente configurado");
}

/* Função: envia payload por LoRaWAN
 * Parâmetros: payload a ser enviado
 * Retorno: nenhum
 */
void envia_payload_lorawan(char *pt_payload)
{
    char cmd_envio_payload[TAM_MAX_CMD_AT] = {0};

    snprintf(cmd_envio_payload, sizeof(cmd_envio_payload), "AT+SENDB=5:%s", pt_payload);
    ESP_LOGI(TAG_LOGS_LORAWAN, "Comando para envio do payload: %s", cmd_envio_payload);
    envia_comando_uart(cmd_envio_payload, strlen(cmd_envio_payload));
    esp_task_wdt_reset();
}