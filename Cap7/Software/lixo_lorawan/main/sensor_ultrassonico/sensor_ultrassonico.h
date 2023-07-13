/* Header file do módulo de sensores */

#ifndef SENSORES_DEFS_H
#define SENSORES_DEFS_H

/* Definição - máxima distância */
#define MAX_DISTANCE_CM        500

/* Estrutura de configuração dos sensores */
typedef struct __attribute__((__packed__))
{
    int gpio_echo;
    int gpio_trigger;
    int gpio_liga_desliga;    
}TConfig_sensores;

#endif

/* Protótipos */
void inicializa_sensor(TConfig_sensores * pt_config_sensores);
void le_sensor(TConfig_sensores * pt_config_sensores, float * pt_distancia_cm);