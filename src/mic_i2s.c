/* Leitura I2S: PIO baseado na implementacao MIT de Ricardo Massaro.
 * https://github.com/moefh/pico-i2s-mic
 * Adaptado para duas areas estaticas e PCM16 mono para Vosk.
 * Reservas: 1 state machine PIO0, 1 canal DMA, DMA_IRQ_0 exclusiva.
 */
#include <stddef.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "mic_i2s.h"
#include "mic_i2s.pio.h"

static PIO pio_inst;
static int sm_index = -1;
static int dma_index = -1;
static uint32_t dma_words[2][MIC_DMA_WORDS];
static volatile unsigned writing_index;
static volatile unsigned finished_index;
static volatile uint32_t finished_seq;
static uint32_t read_seq;
static uint32_t dropped;

static void __isr mic_dma_handler(void) {
    uint32_t irq_bit = (uint32_t)1u << (unsigned)dma_index;
    if ((dma_hw->ints0 & irq_bit) == 0) return;
    dma_hw->ints0 = irq_bit; 

    unsigned completed = writing_index;
    writing_index ^= 1u;
    dma_hw->ch[dma_index].al2_write_addr_trig = (uintptr_t)dma_words[writing_index];
    finished_index = completed;
    __dmb();
    ++finished_seq;
}

bool mic_i2s_init(unsigned sck_gpio, unsigned ws_gpio, unsigned data_gpio) {
    if (ws_gpio != sck_gpio + 1u || sck_gpio >= 29u || data_gpio >= 30u ||
        data_gpio == sck_gpio || data_gpio == ws_gpio) return false;
    pio_inst = pio0;
    sm_index = pio_claim_unused_sm(pio_inst, false);
    if (sm_index < 0) return false;
    if (!pio_can_add_program(pio_inst, &mic_i2s_program)) return false;
    unsigned offset = pio_add_program(pio_inst, &mic_i2s_program);

    pio_sm_config config = mic_i2s_program_get_default_config(offset);
    sm_config_set_in_pins(&config, data_gpio);
    sm_config_set_sideset_pins(&config, sck_gpio);
    sm_config_set_in_shift(&config, false, true, 32);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_RX);
    float div = (float)clock_get_hz(clk_sys) / (128.0f * (float)MIC_SAMPLE_RATE);
    sm_config_set_clkdiv(&config, div);

    pio_gpio_init(pio_inst, sck_gpio);
    pio_gpio_init(pio_inst, ws_gpio);
    pio_gpio_init(pio_inst, data_gpio);
    gpio_pull_down(data_gpio); /* SD alta impedancia no canal nao escolhido */
    pio_sm_init(pio_inst, (unsigned)sm_index, offset, &config);
    uint32_t outs = (1u << sck_gpio) | (1u << ws_gpio);
    pio_sm_set_pindirs_with_mask(pio_inst, (unsigned)sm_index,
                                 outs, outs | (1u << data_gpio));

    dma_index = dma_claim_unused_channel(false);
    if (dma_index < 0) return false;
    dma_channel_config dc = dma_channel_get_default_config((unsigned)dma_index);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_read_increment(&dc, false);
    channel_config_set_write_increment(&dc, true);
    channel_config_set_dreq(&dc, pio_get_dreq(pio_inst, (unsigned)sm_index, false));

    writing_index = finished_index = 0;
    finished_seq = read_seq = dropped = 0;
    dma_channel_configure((unsigned)dma_index, &dc,
                          dma_words[0], &pio_inst->rxf[sm_index],
                          MIC_DMA_WORDS, false);
    dma_channel_set_irq0_enabled((unsigned)dma_index, true);
    irq_set_exclusive_handler(DMA_IRQ_0, mic_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    return true;
}

void mic_i2s_start(void) {
    if (sm_index < 0 || dma_index < 0) return;
    pio_sm_set_enabled(pio_inst, (unsigned)sm_index, false);
    pio_sm_clear_fifos(pio_inst, (unsigned)sm_index);
    pio_sm_restart(pio_inst, (unsigned)sm_index);
    dma_channel_start((unsigned)dma_index);
    pio_sm_set_enabled(pio_inst, (unsigned)sm_index, true);
}

bool mic_i2s_try_read_pcm(uint8_t pcm[MIC_PCM_BYTES], uint32_t *nivel) {
    if (dma_index < 0 || pcm == NULL) return false;
    uint32_t seq = finished_seq;
    if (seq == read_seq) return false;
    __dmb();
    unsigned idx = finished_index;
    if ((uint32_t)(seq - read_seq) > 1u)
        dropped += (uint32_t)(seq - read_seq - 1u);

    uint64_t abs_total = 0;
    for (unsigned i = 0; i < MIC_FRAMES_PER_BLOCK; ++i) {
        uint32_t sample_word = dma_words[idx][2u * i]; /* L/R=GND, WS=LOW */
        /* I2S: sinal do INMP441 no bit 30 (atraso de 1 bit).
         * Corrige alinhamento, extrai 24 bits assinados, reduz p/ PCM16.
         */
        int32_t pcm24 = ((int32_t)(sample_word << 1)) >> 8;
        int16_t pcm16 = (int16_t)(pcm24 >> 8);
        pcm[2u*i] = (uint8_t)((uint16_t)pcm16 & 0xffu);
        pcm[2u*i+1u] = (uint8_t)(((uint16_t)pcm16 >> 8) & 0xffu);
        int32_t v = pcm16;
        abs_total += (uint32_t)(v < 0 ? -v : v);
    }
    __dmb();
    /* Se outra IRQ chegou durante a copia, o resultado pode ter sido sobrescrito.
       Descartamos o bloco em vez de transmitir audio corrompido */
    if (finished_seq != seq) {
        ++dropped;
        read_seq = seq;
        return false;
    }
    read_seq = seq;
    if (nivel) *nivel = (uint32_t)(abs_total / MIC_FRAMES_PER_BLOCK);
    return true;
}
uint32_t mic_i2s_dropped_blocks(void) { return dropped; }
