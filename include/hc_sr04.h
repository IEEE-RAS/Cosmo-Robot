
#ifndef COSMO_HC_SR04_H
#define COSMO_HC_SR04_H

#include "pico/types.h"

typedef enum {
    HC_SR04_SEM_AMOSTRA = 0,
    HC_SR04_VALIDO,
    HC_SR04_SEM_ECO
} hc_sr04_result_t;

void hc_sr04_init(uint trig_pin, uint echo_pin);

hc_sr04_result_t hc_sr04_poll(float *centimetros);

#endif
