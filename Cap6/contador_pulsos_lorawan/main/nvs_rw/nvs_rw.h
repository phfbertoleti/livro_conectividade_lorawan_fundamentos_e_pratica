/* Header file: header file do módulo NVS (Non-Volatile Storage), 
                partição da flash para salvar dados e configurações 
                do projeto.
*/

#ifndef HEADER_MODULO_NVS_RW
#define HEADER_MODULO_NVS_RW

/* Chaves dos contadores */
#define CHAVE_NVS_CONTADOR_1         "c1"
#define CHAVE_NVS_CONTADOR_2         "c2"

/* Definição - numero de envios que força a gravação do contador na NVS */
#define NUM_ENVIOS_PARA_GRAVAR_CONTADORES_NVS          10

#endif

/* Protótipos */
void init_nvs(void);
esp_err_t grava_valor_contador_nvs(char *pt_key, uint32_t valor);
esp_err_t le_valor_contador_nvs(char *pt_key, uint32_t * pt_valor);
esp_err_t limpa_nvs(void);