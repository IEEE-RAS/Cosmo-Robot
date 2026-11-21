#!/usr/bin/env python3
"""Cosmo: INMP441 -> USB CDC -> Vosk pt-BR -> comandos dos olhos.

Usa a mesma porta USB nos dois sentidos:
Pico -> notebook: pacotes de audio PCM16LE (16 kHz, mono)
notebook -> Pico: 'E:FELIZ\\n' / 'A:PISCAR\\n'....

--simulate permite testar TODAS as reacoes sem baixar o modelo Vosk.
"""
from __future__ import annotations

import argparse
import json
import queue
import re
import sys
import threading
import unicodedata
from pathlib import Path

MAGIC = b"\xc0\x5a\xa5\x0d"
PCM_LEN = 2048
SAMPLE_RATE = 16000

REACTIONS = {
    "NORMAL": ("normal", "neutro", "calma", "relaxa"),
    "FELIZ": ("feliz", "alegre", "sorria", "sorrir"),
    "BRAVO": ("bravo", "brava", "raiva", "irritado"),
    "TRISTE": ("triste", "tristeza", "chorando"),
    "SURPRESO": ("surpreso", "surpresa", "surpreendido"),
    "CONFUSO": ("confuso", "confusa", "duvida", "perdido"),
    "SONOLENTO": ("sonolento", "sono", "dormir", "cansado"),
    "ANIMADO": ("animado", "animada", "empolgado", "empolgada"),
    "APAIXONADO": ("apaixonado", "apaixonada", "amor", "coracao"),
    "CURIOSO": ("curioso", "curiosa", "curiosidade"),
    "ASSUSTADO": ("assustado", "assustada", "medo", "susto"),
    "ATENTO": ("atento", "atencao", "presta atencao"),
}
ACTIONS = {
    "PISCAR": ("pisca", "piscar", "piscadinha"),
    "ESQUERDA": ("esquerda", "olha pra esquerda", "olhe para esquerda"),
    "DIREITA": ("direita", "olha pra direita", "olhe para direita"),
    "CENTRO": ("centro", "olha pra frente", "olhe pra frente"),
}


def normalize(text: str) -> str:
    text = unicodedata.normalize("NFD", text.lower())
    text = "".join(c for c in text if unicodedata.category(c) != "Mn")
    return re.sub(r"\s+", " ", re.sub(r"[^a-z0-9 ]", " ", text)).strip()


def decode_command(phrase: str, *, require_wake: bool = False) -> str | None:
    normalized = normalize(phrase)
    if require_wake and not re.search(r"\bcosmos?\b", normalized):
        return None
    # Em caso de multiplos comandos, prioriza o primeiro nome de emocao citado.
    matches: list[tuple[int, str]] = []
    for name, aliases in REACTIONS.items():
        for alias in aliases:
            match = re.search(r"\b" + re.escape(normalize(alias)) + r"\b", normalized)
            if match:
                matches.append((match.start(), f"E:{name}"))
    for name, aliases in ACTIONS.items():
        for alias in aliases:
            match = re.search(r"\b" + re.escape(normalize(alias)) + r"\b", normalized)
            if match:
                matches.append((match.start(), f"A:{name}"))
    return min(matches)[1] if matches else None


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ (0x1021 if crc & 0x8000 else 0)) & 0xFFFF
    return crc


class PacketParser:
    """Aceita bytes parciais e ressincroniza caso haja perda de pacote."""

    def __init__(self):
        self.buffer = bytearray()
        self.bad_crc = 0

    def feed(self, data: bytes) -> list[tuple[int, bytes]]:
        self.buffer.extend(data)
        complete: list[tuple[int, bytes]] = []
        while True:
            start = self.buffer.find(MAGIC)
            if start < 0:
                if len(self.buffer) > len(MAGIC) - 1:
                    del self.buffer[: -(len(MAGIC) - 1)]
                break
            if start:
                del self.buffer[:start]
            if len(self.buffer) < 8:
                break
            length = int.from_bytes(self.buffer[4:6], "little")
            if length != PCM_LEN:
                del self.buffer[0]
                continue
            end = 8 + length + 2
            if len(self.buffer) < end:
                break
            seq = int.from_bytes(self.buffer[6:8], "little")
            payload = bytes(self.buffer[8:8+length])
            check = int.from_bytes(self.buffer[8+length:end], "little")
            if crc16(payload) == check:
                complete.append((seq, payload))
                del self.buffer[:end]
            else:
                self.bad_crc += 1
                del self.buffer[0]
        return complete


def available_commands() -> list[str]:
    return [f"E:{name}" for name in REACTIONS] + [f"A:{name}" for name in ACTIONS]


def input_thread(commands: queue.Queue):
    print("[Simulacao] Digite uma reacao, ex.: FELIZ, BRAVO, SONOLENTO, PISCAR, DIREITA.")
    print("[Simulacao] 'lista' lista comandos; 'sair' encerra.")
    while True:
        try:
            typed = input("cosmo> ").strip()
        except (EOFError, KeyboardInterrupt):
            commands.put(None)
            return
        if typed.lower() in ("sair", "exit", "quit"):
            commands.put(None)
            return
        if typed.lower() == "lista":
            print("\n".join(available_commands()))
            continue
        canonical = typed.upper()
        if canonical not in available_commands():
            parsed = decode_command(typed)
            canonical = parsed or ""
        if canonical:
            commands.put(canonical)
        else:
            print("Nao reconheci a reacao. Digite 'lista'.")


def main() -> int:
    ap = argparse.ArgumentParser(description="Cosmo RP2040: reconhecimento offline pt-BR")
    ap.add_argument("--port", required=True, help="Ex.: /dev/ttyACM0")
    ap.add_argument("--model", default="models/vosk-model-small-pt-0.3", help="Pasta do modelo Vosk")
    ap.add_argument("--simulate", action="store_true", help="Testar expressoes digitando, sem Vosk")
    ap.add_argument("--require-wake", action="store_true", help="Exigir a palavra Cosmo no comando (pode falhar mais)")
    ap.add_argument("--verbose", action="store_true", help="Mostrar transcricoes e estatisticas")
    args = ap.parse_args()

    try:
        import serial
    except ImportError as e:
        print("Falta pyserial: pip install -r host/requirements.txt", file=sys.stderr)
        raise SystemExit(2) from e

    recognizer = None
    if not args.simulate:
        try:
            from vosk import Model, KaldiRecognizer, SetLogLevel
        except ImportError as e:
            print("Falta vosk: pip install -r host/requirements.txt", file=sys.stderr)
            raise SystemExit(2) from e
        path = Path(args.model)
        if not path.is_dir():
            print(f"Modelo nao encontrado: {path}. Baixe o modelo pequeno pt-BR (ver README).", file=sys.stderr)
            return 2
        SetLogLevel(-1)
        recognizer = KaldiRecognizer(Model(str(path)), SAMPLE_RATE)
        print(f"[Vosk] Modelo carregado: {path}")

    parser = PacketParser()
    q: queue.Queue = queue.Queue()
    if args.simulate:
        threading.Thread(target=input_thread, args=(q,), daemon=True).start()

    try:
        with serial.Serial(args.port, baudrate=115200, timeout=0.05, write_timeout=1.0) as port:
            port.dtr = True
            print(f"[USB] Conectado a {args.port} | {SAMPLE_RATE} Hz PCM16 mono")
            print("[USB] A serial transmite AUDIO BINARIO; nao abra minicom ao mesmo tempo.")
            if not args.simulate:
                print("[Voz] Diga: 'feliz', 'bravo', 'triste', 'surpreso', 'confuso' ...")
            last_seq = None
            packets = 0
            while True:
                if args.simulate:
                    try:
                        cmd = q.get_nowait()
                    except queue.Empty:
                        cmd = ""
                    if cmd is None:
                        return 0
                    if cmd:
                        port.write((cmd + "\n").encode("ascii"))
                        print(f"[Teste] -> {cmd}")
                incoming = port.read(8192)
                for seq, samples in parser.feed(incoming):
                    if last_seq is not None and seq != ((last_seq + 1) & 0xffff) and args.verbose:
                        print(f"[USB] lacuna de sequencia: anterior={last_seq}, agora={seq}")
                    last_seq = seq
                    packets += 1
                    if recognizer and recognizer.AcceptWaveform(samples):
                        phrase = json.loads(recognizer.Result()).get("text", "")
                        if phrase:
                            command = decode_command(phrase, require_wake=args.require_wake)
                            if args.verbose or command:
                                print(f"[Vosk] {phrase!r} -> {command or 'nenhum comando'}")
                            if command:
                                port.write((command + "\n").encode("ascii"))
                    if args.verbose and packets % 100 == 0:
                        print(f"[USB] {packets} pacotes validos, CRC incorreto={parser.bad_crc}")
    except KeyboardInterrupt:
        print("\n[Cosmo] Encerrado.")
        return 0
    except serial.SerialException as e:
        print(f"Erro serial: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
