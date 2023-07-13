/* Header file: LoRaWAN */

#ifndef HEADER_COMM_LORAWAN
#define HEADER_COMM_LORAWAN

/* Definições - GPIOs utilizados na comunicação
                serial com módulo LoRaWAN
*/
#define GPIO_COMM_UART_MOD_LORAWAN_TX      GPIO_NUM_23
#define GPIO_COMM_UART_MOD_LORAWAN_RX      GPIO_NUM_5
#define GPIO_COMM_UART_MOD_LORAWAN_RTS     (UART_PIN_NO_CHANGE)
#define GPIO_COMM_UART_MOD_LORAWAN_CTS     (UART_PIN_NO_CHANGE)

/* Definições - UART */
#define TAM_BUFFER_UART_MOD_LORAWAN        256  //bytes
#define PORTA_UART_MOD_LORAWAN             UART_NUM_1
#define UART_BAUD_RATE                     9600
#define TEMPO_ESPERA_RECEBE_BYTES          20 / portTICK_PERIOD_MS

/* Definições - LoRaWAN */
#define TEMPO_ENTRE_TRANSMISSOES          900000  //ms ( = 15 minutos)

#endif

/* Protótipos */
void init_lorawan(void);
void envia_mensagem_binaria_lorawan_ABP(char * pt_bytes, int qtde_bytes);