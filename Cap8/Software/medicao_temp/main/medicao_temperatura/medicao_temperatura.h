/* Header file: medição de temperatura */
#ifndef HEADER_MEDICAO_TEMP
#define HEADER_MEDICAO_TEMP

/* Definições - sensor de temperatura */
#define TEMPO_BURN_IN_SENSOR_TEMP                     300000 //ms ( = 5 minutos)
#define TEMPO_ENTRE_LEITURAS_SUCESSIVAS_TEMPERATURA   10000  //ms
#define GPIO_ONE_WIRE_SENSOR_TEMPERATURA              3
#define QTDE_AMOSTRAS_TEMPERATURA                     (TEMPO_ENTRE_TRANSMISSOES/TEMPO_ENTRE_LEITURAS_SUCESSIVAS_TEMPERATURA)
#define GPIO_SENSOR_DS18B20                           4
#define QTDE_MAX_SENSORES_DS18B20                     1

#endif

/* Protótipos */
void init_medicao_temperatura(void);
void reinicializa_medicoes_temperatura(void);
void le_temperatura_atual_e_insere_buffer(void);
int8_t calcula_desvio_padrao_x10(void);
int8_t obtem_temperatura_maxima(void);
int8_t obtem_temperatura_minima(void);
int8_t obtem_media_temperaturas(void);
int quantidade_de_temperaturas_lidas(void);