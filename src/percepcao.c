
#include "percepcao.h"
#include "ir_obstaculo.h"

#define CONFIRMACOES 3u
#define AMOSTRAS_MEDIANA 3u
#define VALIDADE_MS 250u

static percepcao_estado_t atual;

static float distancias[AMOSTRAS_MEDIANA];
static unsigned quantidade;
static unsigned indice;

static zona_prox_t candidata;
static unsigned repeticoes;

static uint32_t ultima_valida_ms;
static bool recebeu_valida;

static float mediana3(float a, float b, float c) {
    if (a > b) {
        float t = a;
        a = b;
        b = t;
    }

    if (b > c) {
        float t = b;
        b = c;
        c = t;
    }

    if (a > b) {
        float t = a;
        a = b;
        b = t;
    }

    return b;
}

static void invalidar_ultrassom(void) {
    atual.ultrassom_valido = false;
    atual.zona = PROX_DESCONHECIDO;

    quantidade = 0;
    indice = 0;

    candidata = PROX_DESCONHECIDO;
    repeticoes = 0;

    recebeu_valida = false;
}

static zona_prox_t classificar(
    float cm,
    zona_prox_t anterior
) {
    // Histerese nas transicoes:
    // muito perto: 8 / 10 cm
    // perto: 20 / 22 cm

    if (anterior == PROX_MUITO_PERTO &&
        cm < 10.0f) {
        return PROX_MUITO_PERTO;
    }

    if (anterior == PROX_PERTO &&
        cm >= 8.0f &&
        cm < 22.0f) {
        return PROX_PERTO;
    }

    if (cm < 8.0f)
        return PROX_MUITO_PERTO;

    if (cm < 20.0f)
        return PROX_PERTO;

    return PROX_LIVRE;
}

void percepcao_init(void) {
    atual = (percepcao_estado_t){
        .zona = PROX_DESCONHECIDO,
        .ultrassom_valido = false,
        .status_ultrassom = HC_SR04_SEM_AMOSTRA,
        .ir_detectado = false
    };

    invalidar_ultrassom();
}

void percepcao_atualizar(uint32_t agora_ms) {
    // Primeiro atualizamos o IR.
    ir_obstaculo_poll(agora_ms);
    atual.ir_detectado = ir_obstaculo_detectado();

    // Depois consultamos o ultrassonico.
    float cm = 0.0f;

    hc_sr04_result_t resultado =
        hc_sr04_poll(&cm);

    if (resultado == HC_SR04_SEM_AMOSTRA) {
        if (!recebeu_valida ||
            (uint32_t)(agora_ms - ultima_valida_ms)
                > VALIDADE_MS) {

            invalidar_ultrassom();
        }

        return;
    }

    atual.status_ultrassom = resultado;

    // Falha nao significa ambiente livre.
    if (resultado != HC_SR04_VALIDO) {
        invalidar_ultrassom();
        return;
    }

    recebeu_valida = true;
    ultima_valida_ms = agora_ms;

    // Buffer circular de tres distancias.
    distancias[indice] = cm;

    indice = (indice + 1u) % AMOSTRAS_MEDIANA;

    if (quantidade < AMOSTRAS_MEDIANA)
        ++quantidade;

    if (quantidade < AMOSTRAS_MEDIANA)
        return;

    atual.ultrassom_valido = true;

    // Remove um valor extremo entre tres amostras.
    atual.distancia_cm = mediana3(
        distancias[0],
        distancias[1],
        distancias[2]
    );

    zona_prox_t leitura = classificar(
        atual.distancia_cm,
        atual.zona
    );

    // Confirma a mesma faixa consecutivamente.
    if (leitura != candidata) {
        candidata = leitura;
        repeticoes = 1u;

    } else if (repeticoes < CONFIRMACOES) {
        ++repeticoes;
    }

    if (repeticoes >= CONFIRMACOES)
        atual.zona = candidata;
}

const percepcao_estado_t *percepcao_obter(void) {
    return &atual;
}
