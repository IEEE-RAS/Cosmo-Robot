#ifndef COSMO_EYES_H
#define COSMO_EYES_H

#include "ssd1306.h"

#define OLHO_LARGURA 35
#define OLHO_ALTURA 25
#define OLHO_ESPACO 18
#define MARGEM_SOBRANCELHA 2
#define ALTURA_SOBRANCELHA 10
#define PISCAR_PASSO 4
#define PISCAR_MIN 2
#define POS_X_BASE ((SSD1306_WIDTH - (2 * OLHO_LARGURA + OLHO_ESPACO)) / 2)
#define POS_Y_BASE ((SSD1306_HEIGHT - OLHO_ALTURA) / 2)

typedef enum {
    NORMAL = 0, FELIZ, BRAVO, TRISTE, SURPRESO, CONFUSO,
    SONOLENTO, ANIMADO, APAIXONADO, CURIOSO, ASSUSTADO, ATENTO,
    EMOCAO_TOTAL
} emocao_t;

void desenhar_olhos(int x, int y, int altura, emocao_t emocao);
void piscar(int x, int y, emocao_t emocao);
void mover_para(int xi, int yi, int xf, int yf, emocao_t emocao);
emocao_t proxima_emocao(emocao_t atual);
void olhos_atraso_ms(unsigned ms);

#endif
