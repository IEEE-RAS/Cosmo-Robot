/* Driver SSD1306 baseado no codigo do Cosmo, branch cozmo_arduino.
 * Apenas a comunicacao I2C foi portada do Wire para o Pico SDK.
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "ssd1306.h"

#define OLED_I2C i2c0
#define OLED_SDA 4u
#define OLED_SCL 5u
#define OLED_I2C_HZ 400000u
#define LOTE_BYTES 128

static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static bool erro_avisado;

static void verificar(int transferidos, int esperado) {
    if (transferidos != esperado && !erro_avisado) {
        erro_avisado = true;
        /* Nao enviar logs pela USB: ela transmite audio binario. */
    }
}

static void enviar_comando(uint8_t comando) {
    const uint8_t pacote[2] = {0x00, comando};
    int r = i2c_write_blocking(OLED_I2C, SSD1306_ADDR, pacote, sizeof(pacote), false);
    verificar(r, sizeof(pacote));
}

static const uint8_t init_seq[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
    0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
};

void ssd1306_init(void) {
    i2c_init(OLED_I2C, OLED_I2C_HZ);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(100);
    erro_avisado = false;
    for (size_t i = 0; i < sizeof(init_seq); ++i) enviar_comando(init_seq[i]);
    ssd1306_limpar();
    ssd1306_atualizar();
}

void ssd1306_limpar(void) { memset(buffer, 0, sizeof(buffer)); }

void ssd1306_pixel(int x, int y, bool cor) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    int idx = x + (y / 8) * SSD1306_WIDTH;
    uint8_t mascara = (uint8_t)(1u << (y % 8));
    if (cor) buffer[idx] |= mascara;
    else buffer[idx] &= (uint8_t)~mascara;
}

void ssd1306_retangulo_preenchido(int x, int y, int largura, int altura, bool cor) {
    for (int py = y; py < y + altura; ++py)
        for (int px = x; px < x + largura; ++px)
            ssd1306_pixel(px, py, cor);
}

static int aresta(int ax, int ay, int bx, int by, int px, int py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void ssd1306_triangulo_preenchido(int x0, int y0, int x1, int y1,
                                  int x2, int y2, bool cor) {
    int min_x = x0, max_x = x0, min_y = y0, max_y = y0;
    if (x1 < min_x) min_x = x1;
    if (x1 > max_x) max_x = x1;
    if (x2 < min_x) min_x = x2;
    if (x2 > max_x) max_x = x2;
    if (y1 < min_y) min_y = y1;
    if (y1 > max_y) max_y = y1;
    if (y2 < min_y) min_y = y2;
    if (y2 > max_y) max_y = y2;
    for (int py = min_y; py <= max_y; ++py) {
        for (int px = min_x; px <= max_x; ++px) {
            int e0 = aresta(x0, y0, x1, y1, px, py);
            int e1 = aresta(x1, y1, x2, y2, px, py);
            int e2 = aresta(x2, y2, x0, y0, px, py);
            bool neg = (e0 < 0) || (e1 < 0) || (e2 < 0);
            bool pos = (e0 > 0) || (e1 > 0) || (e2 > 0);
            if (!(neg && pos)) ssd1306_pixel(px, py, cor);
        }
    }
}

void ssd1306_atualizar(void) {
    enviar_comando(0x21); enviar_comando(0); enviar_comando(SSD1306_WIDTH - 1);
    enviar_comando(0x22); enviar_comando(0); enviar_comando(SSD1306_HEIGHT / 8 - 1);

    uint8_t pacote[LOTE_BYTES + 1];
    pacote[0] = 0x40; // Dados graficos
    for (int pagina = 0; pagina < SSD1306_HEIGHT / 8; ++pagina) {
        for (int coluna = 0; coluna < SSD1306_WIDTH; coluna += LOTE_BYTES) {
            int idx = pagina * SSD1306_WIDTH + coluna;
            memcpy(&pacote[1], &buffer[idx], LOTE_BYTES);
            int r = i2c_write_blocking(OLED_I2C, SSD1306_ADDR,
                                       pacote, sizeof(pacote), false);
            verificar(r, sizeof(pacote));
        }
    }
}
