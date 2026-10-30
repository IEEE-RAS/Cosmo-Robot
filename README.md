# Cozmo Robot: teste de bancada no Arduino

Esta branch tem **só o teste de bancada** do robô, feito num Arduino para validar o display OLED e a lógica dos olhos enquanto a Raspberry Pi Zero W não chegava. **Não é o projeto final.** O projeto principal (Linux, Raspberry Pi Zero W, em C puro) está na branch `main/` do repositório.

Robô de mesa inspirado no Cozmo (Anki), projeto via IEEE RAS UEFS.

## O que faz

- Desenha dois olhos num display OLED SSD1306 128x64.
- Pisca a cada 1 a 3 s e, às vezes, move o olhar para uma posição aleatória.
- O botão cicla a emoção: `NORMAL`, `BRAVO`, `TRISTE`.

## Hardware

- Arduino Uno ou compatível
- Display OLED SSD1306 128x64, I2C, endereço `0x3C`
- Push Botton

### Ligações

| Componente | Sinal | Arduino |
| --- | --- | --- |
| Display | SDA | SDA (ou A4) |
| Display | SCL | SCL (ou A5) |
| Display | VCC e GND | confira a tensão do seu módulo |
| Botão | um terminal | 02 |
| Botão | outro terminal | GND |

O botão usa o pull-up interno (`INPUT_PULLUP`): vai direto entre 02 e GND, sem resistor. Num botão de 4 pernas, use duas pernas em diagonal; duas pernas do mesmo lado costumam estar em curto dentro do botão.

Confira a ordem dos pinos escrita no seu módulo OLED (alguns são `GND VCC SCL/SCK SDA`, outros `VCC GND SCL/SCK SDA`). Inverter VCC e GND pode danificar o display.

## Como usar

1. Coloque os arquivos numa pasta chamada **`cozmo_arduino`**, o mesmo nome do `.ino` (a Arduino IDE exige isso; se ela pedir para mover o sketch para uma pasta, aceite).
2. Abra `cozmo_arduino.ino` na Arduino IDE (testado na 1.8.19).
3. Selecione a placa e a porta. Neste clone, a gravação só funcionou a **57600 baud** (bootloader antigo). A 115200 dá `avrdude: not in sync`.
4. Antes do projeto, grave o exemplo **Blink** para confirmar que placa, cabo e porta estão certos.
5. Abra o Monitor Serial a **115200**. Se aparecer `erro I2C: 2` ou `3`, o display não respondeu: confira fiação, alimentação e endereço (`0x3C` ou `0x3D`; se for `0x3D`, mude `SSD1306_ADDR` em `ssd1306.h`).

Se o display não acender, um scanner I2C simples (exemplo comum da comunidade) mostra se ele está no barramento.

## Arquivos

| Arquivo | Função |
| --- | --- |
| `cozmo_arduino.ino` | `setup`/`loop` e o gancho `olhos_atraso_ms` (usa `delay`) |
| `eyes.c` / `eyes.h` | Lógica dos olhos, em C puro e independente de hardware |
| `ssd1306.cpp` / `ssd1306.h` | Driver do display via `Wire` |
| `botao.cpp` / `botao.h` | Leitura do botão com debounce de 30 ms |

### Por que C e C++ juntos

`eyes.c` é C puro e é o **mesmo código** do projeto principal em `linux/`. Ele só desenha no buffer e chama o gancho `olhos_atraso_ms`. Já `Wire`, `Serial` e `millis` são bibliotecas C++ do Arduino, então o display e o botão são `.cpp`. Os headers usam `extern "C"` para os dois lados se enxergarem. A mistura existe só nesta branch.

Se `eyes.c` mudar, atualize a cópia do projeto principal (e o contrário): a fonte de verdade é `linux/`.

## Diferenças em relação à versão Linux

- O envio ao display é fatiado em blocos de 16 bytes, porque o buffer do `Wire` no AVR tem 32 bytes. No Linux, o framebuffer inteiro vai de uma vez.
- O I2C roda a 400 kHz, para as animações não ficarem lentas.
- O botão é lido por polling com `millis()`. O Linux usa a API GPIO v2 do kernel, que enfileira os eventos. Aqui, um clique que começa e termina durante `piscar` ou `mover_para` é perdido.
- `int` tem 16 bits no AVR. Os limites das contas de desenho foram conferidos para o display de 128x64; se mudar as dimensões, confira de novo.

## Uso de memória (medido)

- Flash: 6646 bytes (20%)
- RAM (variáveis globais): 1448 bytes (70% dos 2048), 600 bytes para pilha. Boa parte é o buffer de 1024 bytes do display. Adicionar muito mais variáveis globais aqui pede reavaliação.

## Limitações conhecidas

- O botão só é lido entre as animações, então a resposta ao clique pode atrasar até cerca de meio segundo.
- A semente do `rand()` vem do ruído de um pino analógico solto (`A0`): serve para animação, não para nada que precise ser imprevisível.
- Não há tratamento de falha do display além da mensagem no Serial
