/* Módulo: LoRaWAN */

/* Includes */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "LoRaWAN.h"
#include "driver/uart.h"
#include "driver/gpio.h"

/* Definição - debug */
#define LORAWAN_TAG "LORAWAN"

/* Definição - tamanho máximo de um comando AT para módulo LoRaWAN */
#define TAM_MAX_CMD_AT_LORAWAN 100

/* Definição - tamanho máximo do payload LoRaWAN */
#define TAM_MAX_PAYLOAD_LORAWAN 80

/* Definição - tamanho máximo da resposta enviada pelo módulo LoRaWAN */
#define TAM_MAX_RESP_MOD_LORAWAN 200

/* Funções locais */
static void envia_bytes_uart(char *pt_bytes, int qtde_bytes);
static void aguarda_e_recebe_resposta_mod_lorawan(char *pt_bytes, int qtde_bytes);

/* Credenciais LoRaWAN */
static const char DEVADDR[] = "00:00:00:00";
static const char APPSKEY[] = "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00";
static const char NWKSKEY[] = "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00";
static const char APPEUI[] = "00:00:00:00:00:00:00:00";


/* Porta LoRaWAN */
static const int porta_lorawan = 12;

/* Função: inicializa LoRaWAN
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void init_lorawan(void)
{
    char cmd_modulo_lorawan[TAM_MAX_CMD_AT_LORAWAN] = {0};
    char resposta_modulo_lorawan[TAM_MAX_RESP_MOD_LORAWAN] = {0};
    int intr_alloc_flags = 0;
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_REF_TICK,
    };

    ESP_LOGI(LORAWAN_TAG, "Inicializando LoRaWAN...");

    /* Inicializa comunicação serial com módulo LoRaWAN */
    ESP_LOGI(LORAWAN_TAG, "Configurando UART para comunicar com MOD_LORAWAN...\n");

    ESP_ERROR_CHECK(uart_driver_install(PORTA_UART_MOD_LORAWAN, TAM_BUFFER_UART_MOD_LORAWAN * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(PORTA_UART_MOD_LORAWAN, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(PORTA_UART_MOD_LORAWAN,
                                 GPIO_COMM_UART_MOD_LORAWAN_TX,
                                 GPIO_COMM_UART_MOD_LORAWAN_RX,
                                 GPIO_COMM_UART_MOD_LORAWAN_RTS,
                                 GPIO_COMM_UART_MOD_LORAWAN_CTS));

    /* Inicializa módulo LoRaWAN */
    /* Acorda módulo */
    ESP_LOGI(LORAWAN_TAG, "Acordando modulo LoRaWAN...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT\n");
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    /* Reseta módulo */
    ESP_LOGI(LORAWAN_TAG, "Reseta modulo LoRaWAN...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "ATZ\n");
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);
    
    /* Configura Join mode para ABP */
    ESP_LOGI(LORAWAN_TAG, "Configurando Join mode em ABP...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+NJM=0\n");
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    /* Configura Classe LoRaWAN como A */
    ESP_LOGI(LORAWAN_TAG, "Configurando classe LoRaWAN como A...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+CLASS=A\n");
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

     /* Configura Device Address */
    ESP_LOGI(LORAWAN_TAG, "Configurando Device Address...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+DADDR=%s\n", DEVADDR);
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    /* Le DEVADDR de volta */
    ESP_LOGI(LORAWAN_TAG, "Lendo Device Address...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+DADDR=?\n");
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Leitura do Device Address: %s", resposta_modulo_lorawan);

    /* Configura Application Session Key */
    ESP_LOGI(LORAWAN_TAG, "Configurando Application Session Key...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+APPSKEY=%s\n", APPSKEY);
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    /* Configura Network Session Key */
    ESP_LOGI(LORAWAN_TAG, "Configurando Network Session Key...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+NWKSKEY=%s\n", NWKSKEY);
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    /* Configura Application EUI */
    ESP_LOGI(LORAWAN_TAG, "Configurando Application EUI...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+APPEUI=%s\n", APPEUI);
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    /* Liga ADR */
    ESP_LOGI(LORAWAN_TAG, "Configurando ADR em 1");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+ADR=1\n");
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    /* Configura DR em DR2 (adequado para o envio de 8 bytes do payload do projeto) */
    ESP_LOGI(LORAWAN_TAG, "Configurando DR em DR2");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+DR=2\n");
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);

    

    ESP_LOGI(LORAWAN_TAG, "LoRaWAN inicializado");
}

/* Função: envia mensagem (binaria) via LoRaWAN (ABP)
 * Parâmetros: - ponteiro para array de bytes a enviar
 *             - quantidade de bytes a serem enviados
 * Retorno: nenhum
 */
void envia_mensagem_binaria_lorawan_ABP(char *pt_bytes, int qtde_bytes)
{
    char cmd_modulo_lorawan[TAM_MAX_CMD_AT_LORAWAN] = {0};
    char resposta_modulo_lorawan[TAM_MAX_RESP_MOD_LORAWAN] = {0};
    char payload[TAM_MAX_PAYLOAD_LORAWAN] = {0};
    char byte_convertido[3] = {0};
    int i = 0;

    /* Se o numero de bytes a serem enviados exceder o limite, nada é feito */
    if (qtde_bytes > TAM_MAX_PAYLOAD_LORAWAN)
    {
        ESP_LOGE(LORAWAN_TAG, "Tamanho do payload (%d) excedeu o tamaho maximo permitido (%d)",
                 qtde_bytes,
                 TAM_MAX_PAYLOAD_LORAWAN);
        return;
    }

    for (i = 0; i < qtde_bytes; i++)
    {
        memset(byte_convertido, 0x00, sizeof(byte_convertido));
        snprintf(byte_convertido, sizeof(byte_convertido), "%02X", pt_bytes[i]);        
        strcat(payload, byte_convertido);
    }

    ESP_LOGI(LORAWAN_TAG, "Enviando mensagem (binaria)...");
    memset(cmd_modulo_lorawan, 0x00, sizeof(cmd_modulo_lorawan));
    memset(resposta_modulo_lorawan, 0x00, sizeof(resposta_modulo_lorawan));
    snprintf(cmd_modulo_lorawan, sizeof(cmd_modulo_lorawan), "AT+SENDB=%d:%s\n", porta_lorawan, payload);
    envia_bytes_uart(cmd_modulo_lorawan, strlen(cmd_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Enviando comando ao modulo LoRaWAN: %s", cmd_modulo_lorawan);
    aguarda_e_recebe_resposta_mod_lorawan(resposta_modulo_lorawan, sizeof(resposta_modulo_lorawan));
    ESP_LOGI(LORAWAN_TAG, "Resposta do modulo LoRaWAN: %s", resposta_modulo_lorawan);
}

/* Função: envia bytes para uart (do módulo LoRaWAN)
 * Parâmetros: - ponteiro para array de bytes a enviar
 *             - quantidade de bytes a serem enviados
 * Retorno: nenhum
 */
static void envia_bytes_uart(char *pt_bytes, int qtde_bytes)
{
    int qtde_bytes_escritos = uart_write_bytes(PORTA_UART_MOD_LORAWAN, pt_bytes, qtde_bytes);
    ESP_LOGI(LORAWAN_TAG, "Quantidade de bytes escritos na UART: %d", qtde_bytes_escritos);
}

/* Função: aguarda e recebe resposta enviada do módulo LoRaWAN
 * Parâmetros: - ponteiro para array de bytes da resposta
 *             - quantidade máxima de bytes permitidos
 * Retorno: nenhum
 */
static void aguarda_e_recebe_resposta_mod_lorawan(char *pt_bytes, int qtde_bytes)
{
    vTaskDelay(500 / portTICK_PERIOD_MS);
    int qtde_bytes_lidos = uart_read_bytes(PORTA_UART_MOD_LORAWAN,
                                           pt_bytes,
                                           qtde_bytes,
                                           TEMPO_ESPERA_RECEBE_BYTES);

    ESP_LOGI(LORAWAN_TAG, "Quantidade de bytes lidos da UART: %d", qtde_bytes_lidos);
}