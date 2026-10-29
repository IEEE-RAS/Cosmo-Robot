#include <Arduino.h>
#include "botao.h"

#define DEBOUNCE_MS 30UL

static uint8_t pino_botao = 0;
static bool nivel_anterior = true;            /* pull-up: solto = HIGH */
static unsigned long ultimo_evento_ms = 0;

void botao_init(unsigned pino)
{
  pino_botao = (uint8_t)pino;
  pinMode(pino_botao, INPUT_PULLUP);
  nivel_anterior = (digitalRead(pino_botao) == HIGH);
  ultimo_evento_ms = millis();
}

bool botao_esperar(unsigned ms)
{
  unsigned long inicio = millis();

  while ((unsigned long)(millis() - inicio) < ms)
  {
    bool nivel = (digitalRead(pino_botao) == HIGH);

    if (nivel != nivel_anterior)
    {
      unsigned long agora = millis();
      /*  ignora evento a menos de 30 ms do anterior */
      bool estavel = (agora - ultimo_evento_ms) > DEBOUNCE_MS;

      ultimo_evento_ms = agora;
      nivel_anterior = nivel;

      if (estavel && !nivel)
      {
        return true;                          /* clique = borda de descida estável */
      }
    }
  }
  return false;
}
