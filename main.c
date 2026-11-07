/* Cosmo: Raspberry Pi Pico + OLED SSD1306 + INMP441
 * Voz reconhecida pelo notebook Vosk via USB CDC
 * O fio USB carrega pacotes de audio BINARIOS no sentido Pico->PC,
 * e comandos ASCII terminados em '\n' no sentido PC->Pico
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "eyes.h"
#include "ssd1306.h"
#include "botao.h"
#include "mic_i2s.h"

#define BTN_GPIO 2u
#define MIC_SCK_GPIO 10u
#define MIC_WS_GPIO 11u
#define MIC_SD_GPIO 12u
#define LIMITE_X 15
#define LIMITE_Y 10
#define DETECTAR_SOM_LIMIAR 700u /* PCM16: ajuste conforme ambiente */
#define ATENCAO_MS 400u
#define REACAO_MS 4500u

static int pos_x = POS_X_BASE;
static int pos_y = POS_Y_BASE;
static int eye_height = OLHO_ALTURA;
static emocao_t emotion = NORMAL;
static bool attention;
static uint32_t attention_until, reaction_until, next_action, next_frame;
static bool reaction_active;
static uint16_t audio_seq;
static uint8_t pcm[MIC_PCM_BYTES];
static char cmd_line[40];
static unsigned cmd_size;

typedef enum { IDLE, BLINK_CLOSING, BLINK_OPENING, MOVING } animation_t;
static animation_t animation = IDLE;
static int start_x, start_y, goal_x, goal_y, move_step;

void olhos_atraso_ms(unsigned ms) { sleep_ms(ms); }
static uint32_t now_ms(void) { return to_ms_since_boot(get_absolute_time()); }
static bool deadline(uint32_t now, uint32_t when) { return (int32_t)(now - when) >= 0; }
static void draw(void) {
    desenhar_olhos(pos_x, pos_y, eye_height, attention ? ATENTO : emotion);
}
static void start_blink(uint32_t now) {
    animation = BLINK_CLOSING;
    next_frame = now;
}
static void start_move(uint32_t now, int x, int y) {
    start_x = pos_x; start_y = pos_y;
    goal_x = x; goal_y = y; move_step = 0;
    animation = MOVING; next_frame = now;
}
static void animate(uint32_t now) {
    if (attention) return;
    if (animation == IDLE) {
        if (deadline(now, next_action)) start_blink(now);
        return;
    }
    if (!deadline(now, next_frame)) return;
    if (animation == BLINK_CLOSING) {
        eye_height -= PISCAR_PASSO;
        if (eye_height <= PISCAR_MIN) {
            eye_height = PISCAR_MIN;
            animation = BLINK_OPENING;
        }
        draw(); next_frame = now_ms() + 18u;
    } else if (animation == BLINK_OPENING) {
        eye_height += PISCAR_PASSO;
        if (eye_height >= OLHO_ALTURA) {
            eye_height = OLHO_ALTURA;
            animation = IDLE;
            if ((rand() % 3) == 0) {
                int tx = POS_X_BASE + (rand() % (2 * LIMITE_X)) - LIMITE_X;
                int ty = POS_Y_BASE + (rand() % (2 * LIMITE_Y)) - LIMITE_Y;
                start_move(now_ms(), tx, ty);
            } else {
                next_action = now_ms() + 1000u + (unsigned)(rand() % 2000);
            }
        }
        draw(); next_frame = now_ms() + 18u;
    } else if (animation == MOVING) {
        ++move_step;
        pos_x = start_x + (goal_x - start_x) * move_step / 12;
        pos_y = start_y + (goal_y - start_y) * move_step / 12;
        draw();
        if (move_step >= 12) {
            animation = IDLE;
            next_action = now_ms() + 900u;
        } else next_frame = now_ms() + 22u;
    }
}

static void apply_emotion(emocao_t state, uint32_t now) {
    emotion = state;
    reaction_active = true;
    reaction_until = now + REACAO_MS;
    attention = false;
    animation = IDLE;
    eye_height = OLHO_ALTURA;
    pos_x = POS_X_BASE; pos_y = POS_Y_BASE;
    next_action = now + 1500u;
    draw();
}
static void process_command(const char *cmd) {
    static const struct { const char *name; emocao_t emotion; } table[] = {
        {"E:NORMAL", NORMAL}, {"E:FELIZ", FELIZ},
        {"E:BRAVO", BRAVO}, {"E:TRISTE", TRISTE},
        {"E:SURPRESO", SURPRESO}, {"E:CONFUSO", CONFUSO},
        {"E:SONOLENTO", SONOLENTO}, {"E:ANIMADO", ANIMADO},
        {"E:APAIXONADO", APAIXONADO}, {"E:CURIOSO", CURIOSO},
        {"E:ASSUSTADO", ASSUSTADO}, {"E:ATENTO", ATENTO}
    };
    uint32_t now = now_ms();
    for (unsigned i = 0; i < sizeof(table)/sizeof(table[0]); ++i) {
        if (strcmp(cmd, table[i].name) == 0) {
            apply_emotion(table[i].emotion, now);
            return;
        }
    }
    if (strcmp(cmd, "A:PISCAR") == 0) {
        attention = false;
        animation = IDLE;
        eye_height = OLHO_ALTURA;
        start_blink(now);
    } else if (strcmp(cmd, "A:ESQUERDA") == 0) {
        attention = false;
        start_move(now, POS_X_BASE - LIMITE_X, POS_Y_BASE);
    } else if (strcmp(cmd, "A:DIREITA") == 0) {
        attention = false;
        start_move(now, POS_X_BASE + LIMITE_X, POS_Y_BASE);
    } else if (strcmp(cmd, "A:CENTRO") == 0) {
        attention = false;
        start_move(now, POS_X_BASE, POS_Y_BASE);
    }
}

static void receive_commands(void) {
    /* Le ate 64 bytes em cada passagem; jamais usa getchar bloqueante. */
    for (int i = 0; i < 64; ++i) {
        int ch = stdio_getchar_timeout_us(0);
        if (ch < 0) break;
        if (ch == '\r') continue;
        if (ch == '\n') {
            cmd_line[cmd_size] = '\0';
            if (cmd_size > 0) process_command(cmd_line);
            cmd_size = 0;
        } else if (ch >= 32 && ch <= 126) {
            if (cmd_size + 1 < sizeof(cmd_line)) cmd_line[cmd_size++] = (char)ch;
            else cmd_size = 0;
        }
    }
}

/* Frame C05AA50D | payload_len(2 LE) | sequence(2 LE) | PCM16LE | CRC16CCITT(2 LE)
 * Pacotes somente quando USB CDC estiver conectado.
 */
static uint16_t crc16(const uint8_t *data, size_t len) {
    uint16_t c = 0xffffu;
    for (size_t i = 0; i < len; ++i) {
        c ^= (uint16_t)data[i] << 8;
        for (unsigned j = 0; j < 8; ++j)
            c = (uint16_t)((c << 1) ^ ((c & 0x8000u) ? 0x1021u : 0u));
    }
    return c;
}
static void send_audio(const uint8_t *audio) {
    uint8_t header[8] = {
        0xc0, 0x5a, 0xa5, 0x0d,
        (uint8_t)(MIC_PCM_BYTES & 0xffu), (uint8_t)(MIC_PCM_BYTES >> 8),
        (uint8_t)(audio_seq & 0xffu), (uint8_t)(audio_seq >> 8)
    };
    ++audio_seq;
    uint16_t check = crc16(audio, MIC_PCM_BYTES);
    uint8_t checksum[2] = {(uint8_t)check, (uint8_t)(check >> 8)};
    stdio_put_string((const char *)header, sizeof(header), false, false);
    stdio_put_string((const char *)audio, MIC_PCM_BYTES, false, false);
    stdio_put_string((const char *)checksum, 2, false, false);
}

int main(void) {
    stdio_init_all();
    srand(42);
    ssd1306_init();
    botao_init(BTN_GPIO);
    draw();

    bool mic_ok = mic_i2s_init(MIC_SCK_GPIO, MIC_WS_GPIO, MIC_SD_GPIO);
    if (mic_ok) mic_i2s_start();
    next_action = now_ms() + 1400u;

    while (true) {
        uint32_t now = now_ms();
        receive_commands();

        if (botao_pressionado()) apply_emotion(proxima_emocao(emotion), now);

        if (mic_ok) {
            uint32_t level = 0;
            if (mic_i2s_try_read_pcm(pcm, &level)) {
                if (!reaction_active && level > DETECTAR_SOM_LIMIAR) {
                    attention_until = now + ATENCAO_MS;
                    if (!attention) {
                        attention = true;
                        animation = IDLE;
                        eye_height = OLHO_ALTURA;
                        draw();
                    }
                }
                if (stdio_usb_connected()) send_audio(pcm);
            }
        }
        if (attention && deadline(now_ms(), attention_until)) {
            attention = false;
            next_action = now_ms() + 150u;
            draw();
        }
        if (reaction_active && deadline(now_ms(), reaction_until))
            reaction_active = false;
        animate(now_ms());
        sleep_ms(1);
    }
}
