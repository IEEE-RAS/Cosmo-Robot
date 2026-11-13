# Evidência de teste: reconhecimento de voz do Cosmo

## Contexto

- **Plataforma:** Raspberry Pi Pico (RP2040) com firmware Cosmo, OLED SSD1306 e microfone digital INMP441.
- **Processamento de fala:** notebook Ubuntu, Vosk offline, modelo `vosk-model-small-pt-0.3`.
- **Comunicação:** USB serial `/dev/ttyACM0`, áudio PCM16 mono, 16 kHz.
- **Origem:** saída de terminal compartilhada pela desenvolvedora; data/hora exatas da execução não constam do próprio log.
- **Registro bruto:** [`reconhecimento-voz.log`](./reconhecimento-voz.log).

## Procedimento

```bash
python host/cosmo_voice.py \
  --port /dev/ttyACM0 \
  --model models/vosk-model-small-pt-0.3 \
  --verbose
```

## Resultados observados

- O log chegou a **9.300 pacotes válidos** e reportou **`CRC incorreto=0`** nos marcos de progresso apresentados. Isso não equivale a um teste de taxa de perda nem de latência.
- A transcrição acionou corretamente os seguintes comandos, segundo o registro:

| Texto reconhecido | Comando |
| --- | --- |
| `feliz` | `E:FELIZ` |
| `bravo` | `E:BRAVO` |
| `triste` | `E:TRISTE` |
| `surpreso` | `E:SURPRESO` |
| `pisca` | `A:PISCAR` |
| `olha para a direita` | `A:DIREITA` |

- Ocorreram também as transcrições `ele` e `breve`, ambas classificadas como `nenhum comando`. São exemplos de reconhecimento não correspondente às ações pretendidas, sem acionamento indevido nessas ocorrências.
- Os comandos falados provocaram as reações esperadas no display.

## Encerramento da sessão

O log termina com a mensagem:

```text
Erro serial: device reports readiness to read but returned no data (device disconnected or multiple access on port?)
```

**Causa:** a Raspberry Pi Pico foi **desconectada manualmente da USB** ao final do teste. Portanto, **não foi observada uma queda espontânea de conexão** neste registro. A mensagem do programa é compatível com o encerramento intencional do dispositivo, e não deve ser tratada como falha de estabilidade.

## Limites da evidência

Este registro comprova a execução de comandos e o fluxo de áudio/comunicação na sessão descrita, mas **não apresenta medidas de precisão, taxa de perda ou latência**. Esses itens exigem testes mais específicos.
