/* Header file do módulo LoRaWAN */

#ifndef LORAWAN_DEFS_H
#define LORAWAN_DEFS_H

/* Definição - tamanho máximo de um comando AT */
#define TAM_MAX_CMD_AT                   150

/* Definição: tempo entre dois comandos AT */
#define TEMPO_ENTRE_COMANDOS_AT     1000  //ms

/* Definições - confirmação de envio */
#define LORAWAN_ENVIO_COM_CONFIRMACAO    '1'
#define LORAWAN_ENVIO_SEM_CONFIRMACAO    '0'

/* Definições - Join mode */
#define LORAWAN_JOIN_MODE_ABP             '0'
#define LORAWAN_JOIN_MODE_OTAA            '1'

/* Definições - ADR */
#define LORAWAN_ADR_DESABILITADO          '0'
#define LORAWAN_ADR_HABILITADO            '1'

/* Definições - ADR */
#define LORAWAN_DR_NIVEL_0                '0'  //maior alcance e menor payload
#define LORAWAN_DR_NIVEL_1                '1'
#define LORAWAN_DR_NIVEL_2                '2'
#define LORAWAN_DR_NIVEL_3                '3'
#define LORAWAN_DR_NIVEL_4                '4'
#define LORAWAN_DR_NIVEL_5                '5'
#define LORAWAN_DR_NIVEL_6                '6' //menor alcance e maior payload

/* Definições - Classe do dispositivo LoRaWAN */
#define LORAWAN_CLASSE_A                  'A'
#define LORAWAN_CLASSE_C                  'C'


/* Estrutura de configuração LoRaWAN */
typedef struct __attribute__((__packed__))
{
    /* Chaves e endereços */
    char APPSKEY[60];
    char NWSKEY[60];
    char APPEUI[30];
    char DEVADDR[15];
    char CHMASK[35];

    /* Confirmação de envio */
    char confirmacao_de_envio;

    /* Join mode */
    char join_mode;

    /* ADR */
    char adr;

    /* DR */
    char dr;

    /* Classe do dispositivo LoRaWAN */
    char classe;
}TConfig_LoRaWAN;

#endif

/* Protótipos */
void inicializa_uart_lorawan(void);
void configurar_lorawan(TConfig_LoRaWAN * pt_lorawan);
void envia_payload_lorawan(char * pt_payload);