
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "hc_sr04.h"

#define INTERVALO_US 70000u
#define TIMEOUT_US 35000u
#define PULSO_MIN_US 116u
#define PULSO_MAX_US 25000u

typedef enum {
    OCIOSO,
    ESPERANDO_SUBIDA,
    ESPERANDO_DESCIDA,
    PRONTO
} estado_t;

static uint trig, echo;

static volatile estado_t estado = OCIOSO;
static volatile uint32_t subida_us;
static volatile uint32_t largura_us;

static uint32_t ultimo_disparo_us;

static void eco_irq(uint gpio, uint32_t eventos) {
    if (gpio != echo)
        return;

    if ((eventos & GPIO_IRQ_EDGE_RISE) &&
        estado == ESPERANDO_SUBIDA) {

        subida_us = time_us_32();
        estado = ESPERANDO_DESCIDA;

    } else if ((eventos & GPIO_IRQ_EDGE_FALL) &&
               estado == ESPERANDO_DESCIDA) {

        largura_us = time_us_32() - subida_us;
        estado = PRONTO;
    }
}

void hc_sr04_init(uint trig_pin, uint echo_pin) {
    trig = trig_pin;
    echo = echo_pin;

    estado = OCIOSO;
    ultimo_disparo_us = time_us_32() - INTERVALO_US;

    gpio_init(trig);
    gpio_set_dir(trig, GPIO_OUT);
    gpio_put(trig, false);

    gpio_init(echo);
    gpio_set_dir(echo, GPIO_IN);
    gpio_disable_pulls(echo);

    gpio_set_irq_enabled_with_callback(
        echo,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        eco_irq
    );
}

hc_sr04_result_t hc_sr04_poll(float *centimetros) {
    uint32_t agora = time_us_32();

    if (estado == PRONTO) {
        uint32_t largura = largura_us;
        estado = OCIOSO;

        if (largura < PULSO_MIN_US ||
            largura > PULSO_MAX_US) {
            return HC_SR04_SEM_ECO;
        }

        if (centimetros) {
            *centimetros = largura / 58.0f;
        }

        return HC_SR04_VALIDO;
    }

    if (estado == ESPERANDO_SUBIDA ||
        estado == ESPERANDO_DESCIDA) {

        if ((uint32_t)(agora - ultimo_disparo_us)
            >= TIMEOUT_US) {

            estado = OCIOSO;
            return HC_SR04_SEM_ECO;
        }

        return HC_SR04_SEM_AMOSTRA;
    }

    if ((uint32_t)(agora - ultimo_disparo_us)
        >= INTERVALO_US && !gpio_get(echo)) {

        ultimo_disparo_us = agora;
        estado = ESPERANDO_SUBIDA;

        gpio_put(trig, true);
        busy_wait_us_32(10u);
        gpio_put(trig, false);
    }

    return HC_SR04_SEM_AMOSTRA;
}
