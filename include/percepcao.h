
#ifndef COSMO_PERCEPCAO_H
#define COSMO_PERCEPCAO_H

#include <stdbool.h>
#include <stdint.h>

#include "hc_sr04.h"

typedef enum {
    PROX_DESCONHECIDO,
    PROX_LIVRE,
    PROX_PERTO,
    PROX_MUITO_PERTO
} zona_prox_t;

typedef struct {
    zona_prox_t zona;

    bool ultrassom_valido;
    float distancia_cm;

    hc_sr04_result_t status_ultrassom;

    bool ir_detectado;
} percepcao_estado_t;

void percepcao_init(void);
void percepcao_atualizar(uint32_t agora_ms);

const percepcao_estado_t *percepcao_obter(void);

#endif
