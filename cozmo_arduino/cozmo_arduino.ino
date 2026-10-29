#include <stdlib.h>
#include "eyes.h"
#include "botao.h"

#define LIMITE_MOVIMENTO_X 15
#define LIMITE_MOVIMENTO_Y 10
#define PIN_BOTAO           2

void olhos_atraso_ms(unsigned ms) { delay(ms); }

static int pos_x = POS_X_BASE;
static int pos_y = POS_Y_BASE;
static emocao_t emocao = NORMAL;

void setup()
{
  Serial.begin(9600);
  ssd1306_init();
  botao_init(PIN_BOTAO);
  srand(analogRead(A0));        /* A0 solto: ruído como semente, suficiente para animação */

  desenhar_olhos(pos_x, pos_y, OLHO_ALTURA, emocao);
}

void loop()
{
  if (botao_esperar(1000 + (rand() % 2000)))
  {
    emocao = proxima_emocao(emocao);
  }

  piscar(pos_x, pos_y, emocao);

  if (rand() % 3 == 0)
  {
    int alvo_x = POS_X_BASE + (rand() % (2 * LIMITE_MOVIMENTO_X)) - LIMITE_MOVIMENTO_X;
    int alvo_y = POS_Y_BASE + (rand() % (2 * LIMITE_MOVIMENTO_Y)) - LIMITE_MOVIMENTO_Y;
    mover_para(pos_x, pos_y, alvo_x, alvo_y, emocao);
    pos_x = alvo_x;
    pos_y = alvo_y;
  }
}
