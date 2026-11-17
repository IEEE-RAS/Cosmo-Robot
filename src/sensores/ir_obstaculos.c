
#include "ir_obstaculo.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define IR_DEBOUNCE_MS 50u
#define IR_ATIVO_EM_LOW 1

static uint ir_pin;
static bool candidato;
static bool confirmado;
static uint32_t candidato_desde_ms;

static bool ler_ir(void) {
    bool nivel = gpio_get(ir_pin);

#if IR_ATIVO_EM_LOW
    return !nivel;
#else
    return nivel;
#endif
}

void ir_obstaculo_init(uint gpio) {
    ir_pin = gpio;

    gpio_init(ir_pin);
    gpio_set_dir(ir_pin, GPIO_IN);
    gpio_pull_up(ir_pin);

    candidato = ler_ir();
    confirmado = false;

    candidato_desde_ms =
        to_ms_since_boot(get_absolute_time());
}

void ir_obstaculo_poll(uint32_t agora_ms) {
    bool leitura = ler_ir();

    if (leitura != candidato) {
        candidato = leitura;
        candidato_desde_ms = agora_ms;
    }

    if (confirmado != candidato &&
        (uint32_t)(agora_ms - candidato_desde_ms)
            >= IR_DEBOUNCE_MS) {

        confirmado = candidato;
    }
}

bool ir_obstaculo_detectado(void) {
    return confirmado;
}
