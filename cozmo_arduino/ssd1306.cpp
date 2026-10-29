#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include "ssd1306.h"

/* Wire no AVR tem buffer de 32 bytes: 1 de controle + 16 de dados cabe com folga */
#define LOTE_BYTES 16

static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static bool erro_avisado = false;

static void verificar(uint8_t estado)
{
  if (estado != 0 && !erro_avisado)
  {
    erro_avisado = true;
    Serial.print(F("erro I2C: "));
    Serial.println(estado);
  }
}

static void enviar_comando(uint8_t comando)
{
  Wire.beginTransmission(SSD1306_ADDR);
  Wire.write((uint8_t)0x00);
  Wire.write(comando);
  verificar(Wire.endTransmission());
}

static const uint8_t init_seq[] PROGMEM = {
  0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
  0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
  0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
};

void ssd1306_init(void)
{
  Wire.begin();
  Wire.setClock(400000);
  delay(100);

  for (size_t i = 0; i < sizeof(init_seq); i++)
  {
    enviar_comando(pgm_read_byte(&init_seq[i]));
  }

  ssd1306_limpar();
  ssd1306_atualizar();
}

void ssd1306_limpar(void)
{
  memset(buffer, 0, sizeof(buffer));
}

void ssd1306_pixel(int x, int y, bool cor)
{
  if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT)
  {
    return;
  }

  int indice = x + (y / 8) * SSD1306_WIDTH;
  uint8_t mascara = (uint8_t)(1 << (y % 8));

  if (cor)
  {
    buffer[indice] |= mascara;
  }
  else
  {
    buffer[indice] &= (uint8_t)~mascara;
  }
}

void ssd1306_retangulo_preenchido(int x, int y, int largura, int altura, bool cor)
{
  for (int py = y; py < y + altura; py++)
  {
    for (int px = x; px < x + largura; px++)
    {
      ssd1306_pixel(px, py, cor);
    }
  }
}

static int funcao_aresta(int ax, int ay, int bx, int by, int px, int py)
{
  return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void ssd1306_triangulo_preenchido(int x0, int y0, int x1, int y1, int x2, int y2, bool cor)
{
  int min_x = x0, max_x = x0, min_y = y0, max_y = y0;

  if (x1 < min_x) { min_x = x1; }
  if (x1 > max_x) { max_x = x1; }
  if (x2 < min_x) { min_x = x2; }
  if (x2 > max_x) { max_x = x2; }
  if (y1 < min_y) { min_y = y1; }
  if (y1 > max_y) { max_y = y1; }
  if (y2 < min_y) { min_y = y2; }
  if (y2 > max_y) { max_y = y2; }

  for (int py = min_y; py <= max_y; py++)
  {
    for (int px = min_x; px <= max_x; px++)
    {
      int e0 = funcao_aresta(x0, y0, x1, y1, px, py);
      int e1 = funcao_aresta(x1, y1, x2, y2, px, py);
      int e2 = funcao_aresta(x2, y2, x0, y0, px, py);

      bool neg = (e0 < 0) || (e1 < 0) || (e2 < 0);
      bool pos = (e0 > 0) || (e1 > 0) || (e2 > 0);

      if (!(neg && pos))
      {
        ssd1306_pixel(px, py, cor);
      }
    }
  }
}

void ssd1306_atualizar(void)
{
  enviar_comando(0x21); enviar_comando(0); enviar_comando(SSD1306_WIDTH - 1);
  enviar_comando(0x22); enviar_comando(0); enviar_comando(7);

  for (int pagina = 0; pagina < 8; pagina++)
  {
    for (int coluna = 0; coluna < SSD1306_WIDTH; coluna += LOTE_BYTES)
    {
      Wire.beginTransmission(SSD1306_ADDR);
      Wire.write((uint8_t)0x40);
      Wire.write(&buffer[pagina * SSD1306_WIDTH + coluna], LOTE_BYTES);
      verificar(Wire.endTransmission());
    }
  }
}
