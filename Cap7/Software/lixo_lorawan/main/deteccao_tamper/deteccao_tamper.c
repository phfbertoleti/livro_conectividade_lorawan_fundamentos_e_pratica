/* Módulo de detecção de tamper */
#include <string.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "deteccao_tamper.h"

/* Tag de debug */
static const char* TAG_LOGS_DETECCAO_TAMPER = "DETECCAO_TAMPER";

/* Função: configura tamper
 * Parâmetros: nenhum
 * Retorno: nenhum 
 */
void configura_tamper(void)
{
    gpio_config_t io_conf_tamper = {};  

    /* Configura tamper */     
    ESP_LOGI(TAG_LOGS_DETECCAO_TAMPER, "Configurando tamper...");
    io_conf_tamper.intr_type = GPIO_INTR_DISABLE;  //Desabilita interrupção    
    io_conf_tamper.mode = GPIO_MODE_INPUT;  //configura GPIO como saída
    io_conf_tamper.pin_bit_mask = (1ULL<<GPIO_TAMPER);  //máscara de bits relativa ao GPIO de saida
    io_conf_tamper.pull_down_en = 0;  //desabilita pull-down
    io_conf_tamper.pull_up_en = 0; //desabilita pull-up
    gpio_config(&io_conf_tamper);
    ESP_LOGI(TAG_LOGS_DETECCAO_TAMPER, "Tamper configurado");
}

/* Função: le o estado do GPIO de tamper
 * Parâmetros: nenhum
 * Retorno: estado lido 
 */
int le_tamper(void)
{
    return gpio_get_level(GPIO_TAMPER);
}