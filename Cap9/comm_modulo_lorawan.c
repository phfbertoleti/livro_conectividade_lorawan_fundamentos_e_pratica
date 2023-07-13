#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h> 

/* Definição - tamanho maximo das strings / chaves */
#define TAM_MAX_CMD_AT                 80
#define TAM_NWSKEY_APPSKEY             60
#define TAM_APPEUI                     30
#define TAM_DEVADDR                    15

/* Definição - tempo de espera entre comandos AT */
#define TEMPO_ESPERA_COMANDO_AT        2000000  //us

/* Função: envia buffer via UART
 * Parâmetros: - file descriptor da UART 
 *             - ponteiro para buffer 
 *             - tamanho do buffer
 * Retorno: 1:  envio ok
 *          0: falha em um dos envios
 */
int envia_uart(int fd_uart, char * pt_buffer, int tam_buffer)
{
    ssize_t bytes_escritos_na_uart = 0;
    int status_escrita = 1;
    
    bytes_escritos_na_uart = write(fd_uart, pt_buffer, tam_buffer);

    if (bytes_escritos_na_uart != tam_buffer)
    {
        status_escrita = 0;
    } 

    usleep(TEMPO_ESPERA_COMANDO_AT);
    return status_escrita;
}

/* Função: configura módulo LoRaWAN para operação ABP
 * Parâmetros: file descriptor da UART 
 * Retorno: 1:  envio ok
 *          0: falha em um dos envios
 */
int configura_modulo_lorawan(int fd_uart)
{
    int status_envio = 0;    
    const char nwskey[TAM_NWSKEY_APPSKEY] = "NNNNNNNNNN\0";
    // Network session key
    const char appskey[TAM_NWSKEY_APPSKEY] = "AAAAAAAAAA\0";
    // Application session key
    const char appeui[TAM_APPEUI] = "AAAAAAAAAA\0";      
    // Application EUI
    const char devaddress[TAM_DEVADDR] = "EEEEEEEEEE\0";
    // Device Address
    const char chmask[35] = "00FF:0000:0000:0000:0000:0000\0";
    // Máscara referente a Everynet (LA915)
    char cmd_at[TAM_MAX_CMD_AT] = {0};                 
    // Buffer de envio de comando AT

    /* Envio do channel mask */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+CHMASK=%s\n\r", chmask);
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Envio do join mode */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+NJM=0\n\r");
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Envio do device address */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+DADDR=%s\n\r", devaddress);
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Envio do application EUI */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);

       snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+APPEUI=%s\n\r", appeui);
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Envio do application session key */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+APPSKEY=%s\n\r", appskey);
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Envio do network session key */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+NWKSKEY=%s\n\r", nwskey);
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Habilita ADR (Automatic Data Rate) */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+ADR=1\n\r");
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Configura Data Rate e SPread Factor para máximo alcance 
       e menor payload. Aqui, o consumo do módulo é o maior 
       possível, porém a chance do payload chegar ao gateway é
       significativamente maior. */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+DR=0\n\r");
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Configura classe A para o LoRaWAN */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+CLASS=A\n\r");
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    /* Desliga confirmação de envio */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+CFM=0\n\r");
    if (!envia_uart(fd_uart, cmd_at, strlen(cmd_at)))
    {
        goto FIM_ENVIO;
    }

    status_envio = 1;

FIM_ENVIO:   
    return status_envio;
}

int main (int argc, char *argv[])
{
    /* Credenciais LoRaWAN 
       Lembre-se de substituir pelas suas!
    */
    char cmd_at[TAM_MAX_CMD_AT] = {0};
    // Buffer de envio de comando AT          
    struct termios options;        
    // Estrutura de configuração da UART
    int porta_lorawan = 5;         
    // Porta LoRaWAN
    int fd = 0;                       
    // File descriptor para enviar dados para a UART
    int result = 0;                   
    // Variável que contém resultados/status das 
    // operações com UART

    /* Abre conexão com a UART para comunicação com módulo
       LoRaWAN, em modo escrita/leitura e modo não
       blocante.
    */
    fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
    if (fd == -1)
    {
        perror("Impossivel se comunicar com a UART: ");
        goto TERMINA_PROGRAMA;
    }

    /* Faz flush dos buffers de escrita e leitura da UART */
    result = tcflush(fd, TCIOFLUSH);
    if (result)
    {
        perror("Impossivel fazer flush dos buffers da UART: ");
        goto TERMINA_PROGRAMA;
    }

    /* Configura UART para 9600/8/N/1 */
    result = tcgetattr(fd, &options);
    if (result)
    {
        perror("Impossivel obter configs atuais da UART: ");
        goto TERMINA_PROGRAMA;
    }

    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;
    cfsetospeed(&options, B9600);
    cfsetispeed(&options, cfgetospeed(&options));
 
    result = tcsetattr(fd, TCSANOW, &options);
   
    if (result)
    {
        perror("Impossivel configurar UART: ");
        goto TERMINA_PROGRAMA;
    }

    /* Configura módulo LoRaWAN */                                                         
    if (!configura_modulo_lorawan(fd))
    {
        perror("Impossivel enviar comandos AT via UART: ");
        goto TERMINA_PROGRAMA;
    }

    /* Faz envio da string de teste */
    memset(cmd_at, '\0', TAM_MAX_CMD_AT);
    snprintf(cmd_at, TAM_MAX_CMD_AT, "AT+SEND=%d:Teste\n\r",               
                                                      porta_lorawan);
    if (!envia_uart(fd, cmd_at, strlen(cmd_at)))
    {
        perror("Impossivel enviar comandos AT via UART: ");
        goto TERMINA_PROGRAMA;
    }

    /* Fecha comunicação UART com módulo LoRaWAN */
    close(fd);

TERMINA_PROGRAMA:
    printf("\n\rPrograma terminado.\n\r");
    return 0;
}