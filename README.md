# Cosmo Robot 🤖

**Robô de mesa inspirado no Cozmo (Anki), desenvolvido no contexto do IEEE RAS da Universidade Estadual de Feira de Santana (UEFS).**

## Demonstração do Cosmo

[Assista à demonstração das três formas de interação](docs/demonstracao/Cosmo-Robot_Demonstracao.mp4)

O Cosmo é um projeto experimental de robótica e sistemas embarcados: uma plataforma para estudar **interfaces com periféricos, animações, programação em C, interação por voz e percepção de proximidade**, construindo e testando cada parte antes de integrá-la ao corpo do robô.

A implementação começou como um **teste de bancada no Arduino Uno**, para validar a lógica dos olhos e o display SSD1306 enquanto a Raspberry Pi Zero W não estava disponível. Em paralelo, foi desenvolvida uma implementação para Linux, com acesso direto aos periféricos do sistema. O protótipo mais recente utiliza **Raspberry Pi Pico (RP2040)**, **INMP441**, **HC-SR04** e um aplicativo Python/Vosk executado no notebook Ubuntu. As reações à proximidade funcionam localmente na Pico, sem notebook.

> **Estado da versão RP2040 (12/11/2026):** display OLED, piscadas, botão, comandos USB e reações automáticas ao HC-SR04 foram testados em bancada. O reconhecimento de voz com Vosk também foi demonstrado com notebook, mas a confiabilidade e a latência ainda precisam ser aprimoradas. O sensor IR ainda não está integrado. A Raspberry Pi Zero W permanece como plataforma prevista.

## Evolução do projeto

| Implementação | Plataforma | Objetivo e características |
| --- | --- | --- |
| [Protótipo Arduino](https://github.com/StheffannyNAlves/Cosmo-Robot/tree/cozmo_arduino) | Arduino Uno | Validar olhos, piscadas, expressões e botão. Driver I²C com `Wire`; lógica de desenho em C puro. |
| [Implementação Linux](https://github.com/StheffannyNAlves/Cosmo-Robot/tree/linux) | Raspberry Pi Zero W (planejada) | Explorar acesso a `/dev/i2c-1`, GPIO por interfaces do Linux e compilação nativa em C. |
| [Experimento Pico anterior](https://github.com/StheffannyNAlves/Cosmo-Robot/tree/pico) | Pico SDK (configuração remota antiga para `pico_w`) | Experimento de olhos e SSD1306; não corresponde ao firmware mais recente descrito abaixo. |
| **Protótipo RP2040 + voz + proximidade** | Raspberry Pi Pico (RP2040) + Ubuntu | 12 expressões, captura I²S via PIO/DMA, USB, comandos por texto/voz e percepção local por HC-SR04. |

### Decisões reaproveitadas da versão Arduino

A primeira implementação separou o **desenho dos olhos** (`eyes.c`) dos **drivers específicos de plataforma**. No Arduino, o desenho era C e os drivers eram C++ porque utilizavam `Wire`, `Serial` e `millis()`. Na implementação Linux, a mesma ideia passou a usar APIs do sistema; no RP2040, usa o **Pico SDK**.

Essa separação permite trocar o hardware sem abandonar a lógica visual, embora as versões atuais do desenho **não sejam idênticas**: o firmware RP2040 ganhou novas expressões e foi ajustado após testes físicos.

Como referência histórica, a versão Arduino registrou **6.646 bytes de flash (20%)** e **1.448 bytes de RAM global (70% dos 2 KB)** no ambiente AVR. Esses números **não** representam o consumo da Pico, nem podem ser usados como benchmark do firmware atual.

## Funcionalidades

- **12 expressões faciais** em um OLED monocromático de 128 × 64 pixels.
- **Animações autônomas** de piscadas e movimentação dos olhos.
- **Botão físico** para percorrer as expressões, com tratamento de *debounce*.
- **Captação digital I²S** do INMP441 usando PIO e DMA do RP2040.
- **Comunicação USB CDC bidirecional** entre a Pico e o notebook.
- **Reconhecimento de comandos em português brasileiro**, offline após a instalação do modelo Vosk.
- **Modo de simulação por texto** para testar comandos sem depender do reconhecimento de voz.
- **HC-SR04:** leitura de distância via TRIG/ECHO, interrupções GPIO, verificação periódica do resultado e reações automáticas com confirmação de três amostras.

### Expressões e ações

| Expressão | Comando | Exemplo de fala |
| --- | --- | --- |
| Normal | `NORMAL` | “Fica normal” |
| Feliz | `FELIZ` | “Cosmo, fica feliz” |
| Bravo | `BRAVO` | “Fica bravo” |
| Triste | `TRISTE` | “Fica triste” |
| Surpreso | `SURPRESO` | “Fica surpreso” |
| Confuso | `CONFUSO` | “Você está confuso?” |
| Sonolento | `SONOLENTO` | “Está com sono?” |
| Animado | `ANIMADO` | “Fica animado” |
| Apaixonado | `APAIXONADO` | “Está apaixonado?” |
| Curioso | `CURIOSO` | “Fica curioso” |
| Assustado | `ASSUSTADO` | “Fica assustado” |
| Atento | `ATENTO` | “Presta atenção” |

Também estão disponíveis os comandos **`PISCAR`**, **`ESQUERDA`**, **`DIREITA`** e **`CENTRO`**.

O reconhecimento é baseado em **palavras-chave presentes na transcrição**. Ele não identifica emoções pelo tom de voz nem realiza uma conversa livre com o robô. A palavra “Cosmo” não é obrigatória por padrão.

## Sensor de proximidade HC-SR04

A RP2040 dispara o **TRIG** e mede a largura do pulso **ECHO** usando interrupções por borda, evitando que o laço principal aguarde ativamente o eco. O driver fica em `src/sensores/hc-sr04.c`, com interface em `include/hc_sr04.h`. As regras de comportamento ficam em `main.c`.

| Distância válida | Expressão |
| --- | --- |
| Abaixo de 8 cm | `SURPRESO` |
| De 8 cm até menos de 20 cm | `CURIOSO` |
| A partir de 20 cm | `NORMAL` |

- Uma faixa precisa de **três leituras consecutivas** para ser confirmada.
- Comandos de expressão por botão/USB têm prioridade temporária de aproximadamente 4,5 segundos; comandos de ação USB usam 1,2 segundo.
- Os limiares são iniciais e ainda não foram calibrados para precisão de distância.
- **Limitação importante:** a implementação atual transforma leitura inválida/ausência de eco em `PROX_LIVRE` na camada de comportamento. Isso **não prova** que o caminho esteja livre; a lógica deve ser corrigida antes de qualquer uso de segurança ou anticolisão.
- O HC-SR04 mede apenas na direção em que aponta; não é sensor de borda/queda.
- O HC-SR04 e as expressões funcionam sem notebook. **Vosk continua dependendo do computador.**

### Conexão elétrica do HC-SR04 convencional

| Sinal HC-SR04 | Raspberry Pi Pico | Pino físico Pico |
| --- | --- | --- |
| VCC | VBUS, 5 V quando alimentada por USB | 40 |
| GND | GND comum | GND disponível |
| TRIG | GP6 | 9 |
| ECHO | GP7, **obrigatoriamente com adaptação para 3,3 V** | 10 |

**Proteção do ECHO:** o ECHO de módulos HC-SR04 convencionais pode chegar a 5 V, mas os GPIOs da RP2040 não aceitam 5 V. Use conversor lógico adequado ou divisor resistivo com um resistor entre ECHO e GP7 e outro entre GP7 e GND (por exemplo, 2,2 kΩ em cima e 3,3 kΩ embaixo geram cerca de 3 V). **Dois resistores apenas em paralelo NÃO formam divisor de tensão.** Confira com multímetro antes de conectar o GPIO.

### Ensaios realizados e pendentes

- **Confirmado em bancada:** medição de distância e mudança automática dos olhos conforme aproximação/afastamento.
- **Pendente:** calibração das faixas, testes de timeout, tratamento explícito para leitura desconhecida, estabilidade prolongada e ensaio completo simultâneo com voz e sensor.

## Arquitetura

```text
             Voz
              │
              ▼
         ┌─────────┐
         │ INMP441 │
         └────┬────┘
              │ I²S
              ▼
    ┌──────────────────────┐
    │ Raspberry Pi Pico    │◄── Botão (GP2)
    │ RP2040 · C/Pico SDK  │◄── HC-SR04 (GP6/GP7)
    │ PIO + DMA            │
    └──────┬────────┬──────┘
           │        ▲
       I²C │        │ USB CDC: comandos
           ▼        │
    ┌──────────┐    │       ┌────────────────────┐
    │ SSD1306  │    └───────│ Notebook · Ubuntu  │
    │ Olhos    │            │ Python + Vosk      │
    └──────────┘            └─────────▲──────────┘
                                     │
                         USB CDC: áudio PCM16
                         (enviado pela Pico)
```

O RP2040 continua responsável pelos periféricos e pelas animações. O notebook interpreta o áudio e retorna comandos de expressão ou movimento. Essa separação permite testar a interface visual independentemente do mecanismo de reconhecimento.

## Hardware e ligações

### Componentes

- Raspberry Pi Pico com **RP2040**;
- display **SSD1306 I²C**, 128 × 64;
- microfone digital **INMP441**;
- sensor ultrassônico **HC-SR04**;
- botão (*push button*);
- protoboard, jumpers e cabo USB **de dados**;
- notebook com Ubuntu para executar o reconhecedor Vosk.

### Pinagem

| Componente | Sinal | Pico | Pino físico |
| --- | --- | --- | --- |
| SSD1306 | SDA | GP4 | 6 |
| SSD1306 | SCL | GP5 | 7 |
| SSD1306 | VCC | 3V3(OUT)¹ | 36 |
| SSD1306 | GND | GND | 38 |
| INMP441 | SCK / BCLK | GP10 | 14 |
| INMP441 | WS / LRCLK | GP11 | 15 |
| INMP441 | SD / DOUT | GP12 | 16 |
| INMP441 | VDD | 3V3(OUT) | 36 |
| INMP441 | GND | GND | 13 |
| INMP441 | L/R | GND (canal esquerdo) | 13² |
| Botão | Terminal 1 | GP2 | 4 |
| Botão | Terminal 2 | GND | 3 |
| HC-SR04 | TRIG | GP6 | 9 |
| HC-SR04 | ECHO | GP7 **via conversão de nível** | 10 |
| HC-SR04 | VCC | VBUS (5 V via USB) | 40 |
| HC-SR04 | GND | GND comum | GND disponível |

¹ Verifique se **seu módulo SSD1306 aceita 3,3 V**. ² L/R compartilha a referência de terra; não precisa ocupar sozinho um segundo pino físico da Pico.

**Atenção:** o INMP441 opera em 3,3 V; não conecte 5 V ao microfone nem aos GPIOs da RP2040. O ECHO do HC-SR04 exige adaptação de nível, conforme seção acima. Use GND comum. O botão utiliza o pull-up interno.

## Tecnologias

| Camada | Tecnologias |
| --- | --- |
| Firmware | C11, Pico SDK, CMake, Ninja |
| Display | SSD1306, I²C |
| Microfone | INMP441, I²S, PIO, DMA |
| Proximidade | HC-SR04, GPIO e interrupções |
| Comunicação | USB CDC, protocolo binário de áudio e comandos ASCII |
| Reconhecimento | Python 3, Vosk, pyserial |
| Testes | `unittest` para o aplicativo Python |

## Estrutura do projeto

```text
Cosmo-Robot/
├── CMakeLists.txt
├── main.c                   # Integração e comportamento
├── include/                 # Cabeçalhos: botao, eyes, ssd1306, mic_i2s, hc_sr04
├── src/
│   ├── botao.c
│   ├── eyes.c
│   ├── ssd1306.c
│   ├── mic_i2s.c
│   ├── mic_i2s.pio
│   └── sensores/
│       └── hc-sr04.c       # Driver ultrassônico
├── host/
│   ├── cosmo_voice.py
│   ├── requirements.txt
│   └── tests/
│       └── test_voice.py
├── docs/
├── creditos/
└── README.md
```

## Compilação e gravação do firmware

**Pré-requisitos:** Pico SDK, toolchain ARM (`arm-none-eabi-gcc`), CMake e Ninja configurados.

Defina o caminho do SDK caso não esteja configurado no ambiente:

```bash
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
```

Na raiz do repositório:

```bash
cmake -S . -B build -G Ninja \
  -DPICO_BOARD=pico \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j"$(nproc)"
```

Com a Pico desconectada, mantenha **BOOTSEL** pressionado enquanto conecta o cabo USB. Copie `build/cosmo.uf2` para a unidade `RPI-RP2` que aparecer no computador. A placa reiniciará com o firmware.

O cabeçalho `mic_i2s.pio.h` é **gerado pelo Pico SDK** durante o build. Não é necessário criá-lo ou copiá-lo para `src/`.

## Configuração do reconhecimento de voz

No Ubuntu, a partir da raiz do repositório:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r host/requirements.txt
```

Baixe um modelo Vosk de português para `models/`:

```bash
mkdir -p models
wget -O models/pt-br.zip \
  https://alphacephei.com/vosk/models/vosk-model-small-pt-0.3.zip
unzip models/pt-br.zip -d models/
```

Localize a porta USB CDC da Pico:

```bash
ls -l /dev/ttyACM*
```

### Teste das expressões sem voz

```bash
python host/cosmo_voice.py --port /dev/ttyACM0 --simulate
```

Digite `lista` para ver os comandos, teste `FELIZ`, `BRAVO`, `TRISTE`, `PISCAR` ou `DIREITA` e use `sair` para encerrar. Esse modo verifica a comunicação USB e o firmware **sem exigir transcrição de fala**.

### Reconhecimento de voz real

```bash
python host/cosmo_voice.py \
  --port /dev/ttyACM0 \
  --model models/vosk-model-small-pt-0.3 \
  --verbose
```

Fale frases curtas próximas ao microfone, fazendo pequenas pausas: “fica feliz”, “fica bravo”, “olha para a direita”. O programa interpreta a transcrição e envia o comando correspondente à Pico.

A opção `--require-wake` exige que a transcrição contenha a palavra “Cosmo” (ou “Cosmos”); é experimental, pois o modelo pode transcrever o nome incorretamente.

**Não abra o Monitor Serial ou `minicom` ao mesmo tempo que o script:** a porta transmite áudio binário, e não apenas mensagens de texto.

## Testes

Execute os testes automatizados do programa Python:

```bash
python -m unittest discover -s host/tests -v
```

Eles verificam a associação de palavras a comandos e aspectos da leitura dos pacotes de áudio; não substituem testes do microfone em bancada.

### Protocolo de comunicação

- **Pico → computador:** áudio mono PCM16LE, 16 kHz, empacotado com cabeçalho, sequência e CRC16-CCITT.
- **Computador → Pico:** comandos ASCII terminados por `\n`, como `E:FELIZ\n` e `A:PISCAR\n`.
- **Sem notebook:** botão, animações locais e reações ao HC-SR04 continuam disponíveis.

## Problemas comuns

| Sintoma | Verificação |
| --- | --- |
| `mic_i2s.pio.h` não encontrado pelo VS Code | Compile uma vez e confira `build/mic_i2s.pio.h`. Configure `compile_commands.json` e o include da pasta `build/`. |
| Referência indefinida para `__dmb` | Confirme `#include "hardware/sync.h"` em `src/mic_i2s.c` e recompile. |
| `.venv/bin/activate` não existe | Crie o ambiente com `python3 -m venv .venv`. |
| `/dev/ttyACM0` não existe | Confira o cabo de dados, a enumeração USB e o nome real da porta. |
| Permissão negada na serial | Verifique se o usuário pertence ao grupo `dialout` e reinicie a sessão após ajustar permissões. |
| Reação por texto funciona, mas voz não | Inspecione `--verbose`; investigue captação I²S, nível de áudio, modelo e transcrição antes de alterar o desenho dos olhos. |
| HC-SR04 não reage | Confira alimentação, TRIG GP6, ECHO GP7 com conversão de nível, GND comum e orientação. |
| HC-SR04 retorna leitura inválida | Confira reflexões, conexões e sinal ECHO. **Não** interprete falha como ausência segura de obstáculo. |

## Estado do desenvolvimento e próximos passos

**Confirmado nos testes do protótipo:** display OLED, piscadas, botão, correção visual das expressões `FELIZ`, `BRAVO` e `TRISTE`, medição de distância com HC-SR04 e reações automáticas do display. Os testes de voz anteriores reportaram 9.300 pacotes válidos sem erro de CRC e comandos reconhecidos no Vosk.

**A validar ou aprimorar:** robustez do áudio I²S, latência do reconhecimento, desempenho sob ruído, integridade dos pacotes durante a atualização do OLED, calibração e tratamento de falhas do HC-SR04, estabilidade conjunta de periféricos e reconhecimento de voz autônomo sem notebook.

A evolução planejada inclui futuramente testar sensores IR (ainda não integrados), integrar os componentes ao corpo do Cosmo, reduzir a dependência do notebook e estudar uma solução de reconhecimento compatível com a plataforma embarcada final, inicialmente prevista como **Raspberry Pi Zero W**. Não há reprodução de voz ou síntese de fala implementada nesta versão.

## Registro de testes e histórico

O [README de bancada no Arduino](https://github.com/StheffannyNAlves/Cosmo-Robot/blob/cozmo_arduino/README.md) documenta o uso da Arduino IDE 1.8.19, o bootloader antigo de um clone que exigia 57.600 baud na gravação, o diagnóstico de erros de I²C e as limitações de resposta do botão durante animações bloqueantes.

Na passagem ao RP2040, o botão ganhou leitura sem bloqueio, o desenho foi ampliado para 12 expressões e o microfone foi integrado por PIO/DMA. As correções de desenho das emoções **BRAVO**, **TRISTE** e **FELIZ** foram verificadas visualmente no protótipo.

O Git mantém acessíveis as implementações anteriores mesmo quando seus arquivos deixam de existir na branch em desenvolvimento. Evite substituir os registros antigos por instruções da Pico: são plataformas diferentes, com dependências e medições distintas.

## Referências e créditos

- [Código e README da versão Arduino](https://github.com/StheffannyNAlves/Cosmo-Robot/tree/cozmo_arduino)
- [Código da implementação Linux](https://github.com/StheffannyNAlves/Cosmo-Robot/tree/linux)
- [Raspberry Pi Pico SDK](https://www.raspberrypi.com/documentation/pico-sdk/index.html)
- [RP2040 — documentação e datasheet](https://www.raspberrypi.com/documentation/microcontrollers/silicon.html)
- [INMP441 — datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/INMP441.pdf)
- [HC-SR04 — datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf)
- [Vosk — modelos de reconhecimento](https://alphacephei.com/vosk/models)
- [moefh/pico-i2s-mic](https://github.com/moefh/pico-i2s-mic) — referência para o programa PIO de captura I²S. Consulte a licença correspondente em `third_party/`.

**Desenvolvimento:** [Stheffanny N. Alves](https://github.com/StheffannyNAlves).

> O código próprio do projeto e os componentes de terceiros podem estar sujeitos a licenças diferentes. Consulte os arquivos de licença do repositório antes de reutilizá-los.
