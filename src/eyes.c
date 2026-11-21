#include "eyes.h"

static void ret(int x, int y, int w, int h, bool cor) {
    ssd1306_retangulo_preenchido(x, y, w, h, cor);
}
static void tri(int x0, int y0, int x1, int y1, int x2, int y2, bool cor) {
    ssd1306_triangulo_preenchido(x0, y0, x1, y1, x2, y2, cor);
}

static void desenhar_um_olho(int x, int y, emocao_t emocao, bool direito) {
    if (emocao == FELIZ) {
        /* Olhos abertos e brilhantes: sem achatar a altura do olho. */
        ret(x, y, OLHO_LARGURA, OLHO_ALTURA, true);
        /* Arredonda os cantos superiores do bloco retangular. */
        ret(x, y, 3, 3, false);
        ret(x + OLHO_LARGURA - 3, y, 3, 3, false);
        /* Pupila escura e reflexo luminoso para um olhar alegre. */
        ret(x + 11, y + 7, 13, 13, false);
        ret(x + 13, y + 9, 5, 5, true);
        /* Sobrancelha levemente erguida. */
        ret(x + 8, y - 4, 19, 2, true);
    } else if (emocao == SURPRESO) {
        ret(x + 6, y - 2, 23, 29, true);
        ret(x + 13, y + 5, 9, 14, false);
    } else if (emocao == SONOLENTO) {
        ret(x, y + 9, OLHO_LARGURA, 7, true);
        ret(x + 3, y + 16, 29, 2, true);
    } else if (emocao == CONFUSO) {
        ret(x + (direito ? 6 : 0), y + (direito ? 9 : 0),
            direito ? 23 : 35, direito ? 12 : 25, true);
        if (direito) tri(x + 3, y + 4, x + 32, y - 4, x + 32, y + 1, true);
    } else if (emocao == APAIXONADO) {
        /* Coracao geometrico por olho. */
        ret(x + 3, y + 3, 13, 10, true);
        ret(x + 19, y + 3, 13, 10, true);
        tri(x + 2, y + 10, x + 33, y + 10, x + 17, y + 27, true);
    } else if (emocao == ANIMADO) {
        ret(x, y + 2, 35, 22, true);
        ret(x + 5, y + 6, 6, 7, false);
        ret(x + 23, y + 5, 5, 7, false);
        ret(x + 14, y + 15, 5, 4, false);
    } else if (emocao == CURIOSO) {
        ret(x, y, OLHO_LARGURA, 25, true);
        ret(x + (direito ? 5 : 20), y + 6, 9, 13, false);
    } else if (emocao == ASSUSTADO) {
        ret(x + 3, y - 2, 29, 29, true);
        ret(x + 13, y + 8, 9, 12, false);
        tri(x, y - 6, x + 11, y - 8, x + 3, y - 2, true);
    } else if (emocao == ATENTO) {
        ret(x, y, 35, 25, true);
        ret(x + 13, y + 5, 9, 14, false);
    } else {
        ret(x, y, OLHO_LARGURA, 25, true);
    }
}

void desenhar_olhos(int x, int y, int altura, emocao_t emocao) {
    int deslocamento_y = (OLHO_ALTURA - altura) / 2;
    int yy = y + deslocamento_y;
    int xx = x + OLHO_LARGURA + OLHO_ESPACO;
    int topo = yy - MARGEM_SOBRANCELHA;
    int base = yy + ALTURA_SOBRANCELHA;

    ssd1306_limpar();
    /* No meio da piscada todas as expressoes se tornam linhas curtas. */
    if (altura < OLHO_ALTURA) {
        ret(x, yy, OLHO_LARGURA, altura, true);
        ret(xx, yy, OLHO_LARGURA, altura, true);
    } else {
        desenhar_um_olho(x, yy, emocao, false);
        desenhar_um_olho(xx, yy, emocao, true);
        if (emocao == BRAVO) {
            /* Corta a parte superior interna dos olhos: sobrancelhas fechadas. */
            tri(x + OLHO_LARGURA + 2, topo, x, topo, x + OLHO_LARGURA + 2, base, false);
            tri(xx - 2, topo, xx + OLHO_LARGURA, topo, xx - 2, base, false);
        } else if (emocao == TRISTE) {
            /* Corta a parte superior externa: sobrancelhas caidas. */
            tri(x - 2, topo, x + OLHO_LARGURA, topo, x - 2, base, false);
            tri(xx + OLHO_LARGURA + 2, topo, xx, topo, xx + OLHO_LARGURA + 2, base, false);
        } else if (emocao == CONFUSO) {
            tri(x - 2, y - 1, x + 32, y - 4, x + 32, y, false);
        }
    }
    ssd1306_atualizar();
}

/* Compatibilidade com as funcoes antigas do Cosmo, que nao sao usadas
 * pelo scheduler da aplicacao com microfone. */
void piscar(int x, int y, emocao_t emocao) {
    for (int h = OLHO_ALTURA - PISCAR_PASSO; h >= PISCAR_MIN; h -= PISCAR_PASSO) {
        desenhar_olhos(x, y, h, emocao);
        olhos_atraso_ms(15);
    }
    for (int h = PISCAR_MIN + PISCAR_PASSO; h < OLHO_ALTURA; h += PISCAR_PASSO) {
        desenhar_olhos(x, y, h, emocao);
        olhos_atraso_ms(15);
    }
    desenhar_olhos(x, y, OLHO_ALTURA, emocao);
}
void mover_para(int xi, int yi, int xf, int yf, emocao_t emocao) {
    for (int i = 1; i <= 12; ++i) {
        desenhar_olhos(xi + (xf - xi) * i / 12,
                      yi + (yf - yi) * i / 12, OLHO_ALTURA, emocao);
        olhos_atraso_ms(20);
    }
}
emocao_t proxima_emocao(emocao_t atual) {
    return (emocao_t)(((int)atual + 1) % EMOCAO_TOTAL);
}
