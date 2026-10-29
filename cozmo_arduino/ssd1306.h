#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT  64
#define SSD1306_ADDR   0x3C

#ifdef __cplusplus
extern "C" {
#endif

void ssd1306_init(void);
void ssd1306_limpar(void);
void ssd1306_atualizar(void);
void ssd1306_pixel(int x, int y, bool cor);
void ssd1306_retangulo_preenchido(int x, int y, int largura, int altura, bool cor);
void ssd1306_triangulo_preenchido(int x0, int y0, int x1, int y1, int x2, int y2, bool cor);

#ifdef __cplusplus
}
#endif

#endif
