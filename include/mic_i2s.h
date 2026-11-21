#ifndef COSMO_MIC_I2S_H
#define COSMO_MIC_I2S_H

#include <stdbool.h>
#include <stdint.h>

#define MIC_SAMPLE_RATE 16000u
#define MIC_FRAMES_PER_BLOCK 1024u
#define MIC_DMA_WORDS (2u * MIC_FRAMES_PER_BLOCK)
#define MIC_PCM_BYTES (2u * MIC_FRAMES_PER_BLOCK)

bool mic_i2s_init(unsigned sck_gpio, unsigned ws_gpio, unsigned data_gpio);
void mic_i2s_start(void);
/* Copia o buffer completo para PCM16 LE, retorna false se nada novo
 * `nivel` recebe a media dos valores absolutos PCM16 (0..32768)
 */
bool mic_i2s_try_read_pcm(uint8_t pcm[MIC_PCM_BYTES], uint32_t *nivel);
uint32_t mic_i2s_dropped_blocks(void);

#endif
