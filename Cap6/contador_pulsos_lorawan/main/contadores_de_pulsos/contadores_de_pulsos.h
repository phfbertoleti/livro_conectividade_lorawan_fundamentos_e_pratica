/* Header file: contadores de pulsos */

#include "freertos/queue.h"

#ifndef HEADER_CONTADORES_DE_PULSOS
#define HEADER_CONTADORES_DE_PULSOS

/* Definições - GPIOs utilizados para recepção dos pulsois */
#define GPIO_CONTADOR_1                  3
#define GPIO_CONTADOR_2                  4

/* Filas para receber quantidade de pulsos a serem transmisidos */
QueueHandle_t fila_contador_pulsos_1;
QueueHandle_t fila_contador_pulsos_2;

#endif

/* Protótipos */
void init_contadores_de_pulsos(void);