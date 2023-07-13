/* Header file do módulo de detecção de tamper */

#ifndef DETECCAO_TAMPER_DEFS_H
#define DETECCAO_TAMPER_DEFS_H

/* Definição -  GPIO que gera tamper */
#define GPIO_TAMPER                      GPIO_NUM_34

/* Definição - tempo para fazer debounce do GPIO de tamper */
#define TEMPO_DEBOUNCE_TAMPER            200 //ms

#endif

/* Prototipos */
void configura_tamper(void);
int le_tamper(void);