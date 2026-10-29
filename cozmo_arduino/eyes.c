#include "eyes.h"

void desenhar_olhos(int x, int y, int altura, emocao_t emocao)
{
    int deslocamento_y = (OLHO_ALTURA - altura) / 2;
    int y_olho = y + deslocamento_y;
    int x_olho_direito = x + OLHO_LARGURA + OLHO_ESPACO;
    int topo = y_olho - MARGEM_SOBRANCELHA;
    int base = y_olho + ALTURA_SOBRANCELHA;

    ssd1306_limpar();
    ssd1306_retangulo_preenchido(x,              y_olho, OLHO_LARGURA, altura, true);
    ssd1306_retangulo_preenchido(x_olho_direito, y_olho, OLHO_LARGURA, altura, true);

    if (emocao == BRAVO)
    {
        ssd1306_triangulo_preenchido(
            x - MARGEM_SOBRANCELHA, topo,
            x + OLHO_LARGURA, topo,
            x - MARGEM_SOBRANCELHA, base,
            false);
        ssd1306_triangulo_preenchido(
            x_olho_direito + OLHO_LARGURA + MARGEM_SOBRANCELHA, topo,
            x_olho_direito, topo,
            x_olho_direito + OLHO_LARGURA + MARGEM_SOBRANCELHA, base,
            false);
    }
    else if (emocao == TRISTE)
    {
        ssd1306_triangulo_preenchido(
            x + OLHO_LARGURA + MARGEM_SOBRANCELHA, topo,
            x, topo,
            x + OLHO_LARGURA + MARGEM_SOBRANCELHA, base,
            false);
        ssd1306_triangulo_preenchido(
            x_olho_direito - MARGEM_SOBRANCELHA, topo,
            x_olho_direito + OLHO_LARGURA, topo,
            x_olho_direito - MARGEM_SOBRANCELHA, base,
            false);
    }

    ssd1306_atualizar();
}

void piscar(int x, int y, emocao_t emocao)
{
    for (int altura = OLHO_ALTURA - PISCAR_PASSO; altura >= PISCAR_MIN; altura -= PISCAR_PASSO)
    {
        desenhar_olhos(x, y, altura, emocao);
        olhos_atraso_ms(15);
    }

    for (int altura = PISCAR_MIN + PISCAR_PASSO; altura < OLHO_ALTURA; altura += PISCAR_PASSO)
    {
        desenhar_olhos(x, y, altura, emocao);
        olhos_atraso_ms(15);
    }
    desenhar_olhos(x, y, OLHO_ALTURA, emocao);
}

void mover_para(int xi, int yi, int xf, int yf, emocao_t emocao)
{
    const int passos = 12;

    for (int i = 1; i <= passos; i++)
    {
        int x = xi + (xf - xi) * i / passos;
        int y = yi + (yf - yi) * i / passos;
        desenhar_olhos(x, y, OLHO_ALTURA, emocao);
        olhos_atraso_ms(20);
    }
}

emocao_t proxima_emocao(emocao_t atual)
{
    switch (atual)
    {
        case NORMAL: return BRAVO;
        case BRAVO:  return TRISTE;
        case TRISTE: return NORMAL;
        default:     return NORMAL;
    }
}
