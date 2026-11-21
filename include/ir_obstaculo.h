
#ifndef COSMO_IR_OBSTACULO_H
#define COSMO_IR_OBSTACULO_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/types.h"

void ir_obstaculo_init(uint gpio);
void ir_obstaculo_poll(uint32_t agora_ms);
bool ir_obstaculo_detectado(void);

#endif
