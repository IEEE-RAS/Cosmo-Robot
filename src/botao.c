/* Leitura NAO BLOQUEANTE do botao, pull-up e debounce de 30 ms. */
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "botao.h"

#define DEBOUNCE_MS 30u
static unsigned pino;
static bool bruto_anterior, estado_estavel;
static uint32_t instante_mudanca;

void botao_init(unsigned gpio) {
    pino = gpio;
    gpio_init(pino);
    gpio_set_dir(pino, GPIO_IN);
    gpio_pull_up(pino);
    bruto_anterior = gpio_get(pino);
    estado_estavel = bruto_anterior;
    instante_mudanca = to_ms_since_boot(get_absolute_time());
}

bool botao_pressionado(void) {
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    bool bruto = gpio_get(pino);
    if (bruto != bruto_anterior) {
        bruto_anterior = bruto;
        instante_mudanca = agora;
    }
    if (bruto != estado_estavel && (uint32_t)(agora - instante_mudanca) >= DEBOUNCE_MS) {
        estado_estavel = bruto;
        return !estado_estavel; // LOW = pressionado
    }
    return false;
}
