"""Testes sem audio/hardware/modelo Vosk."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from cosmo_voice import (MAGIC, PCM_LEN, PacketParser, available_commands,
                         crc16, decode_command)


class TestCommandMapping(unittest.TestCase):
    def test_all_emotions_and_actions_have_own_command(self):
        for command in available_commands():
            self.assertTrue(command.startswith(("E:", "A:")))

    def test_portuguese_phrases(self):
        for phrase, expected in [
            ("fica feliz", "E:FELIZ"),
            ("Cosmo, fica bravo", "E:BRAVO"),
            ("estou triste", "E:TRISTE"),
            ("me olha com surpresa", "E:SURPRESO"),
            ("está confuso?", "E:CONFUSO"),
            ("tá com sono", "E:SONOLENTO"),
            ("fica animado", "E:ANIMADO"),
            ("amor", "E:APAIXONADO"),
            ("curioso", "E:CURIOSO"),
            ("assustado", "E:ASSUSTADO"),
            ("atenção", "E:ATENTO"),
            ("fica normal", "E:NORMAL"),
            ("pisca", "A:PISCAR"),
            ("olha pra esquerda", "A:ESQUERDA"),
            ("olha pra direita", "A:DIREITA"),
            ("olha pra frente", "A:CENTRO"),
        ]:
            self.assertEqual(decode_command(phrase), expected, phrase)

    def test_wake_optional(self):
        self.assertIsNone(decode_command("feliz", require_wake=True))
        self.assertEqual(decode_command("Cosmo, feliz", require_wake=True), "E:FELIZ")


class TestPacketParser(unittest.TestCase):
    def test_partial_chunks_and_crc(self):
        pcm = bytes(i % 256 for i in range(PCM_LEN))
        frame = (MAGIC + PCM_LEN.to_bytes(2, "little") +
                 (42).to_bytes(2, "little") + pcm + crc16(pcm).to_bytes(2, "little"))
        parser = PacketParser()
        self.assertEqual(parser.feed(b"garbage" + frame[:4]), [])
        self.assertEqual(parser.feed(frame[4:200]), [])
        self.assertEqual(parser.feed(frame[200:]), [(42, pcm)])
        self.assertEqual(parser.bad_crc, 0)

    def test_bad_packet_resynchronization(self):
        pcm = b"\x00" * PCM_LEN
        frame = MAGIC + PCM_LEN.to_bytes(2, "little") + b"\x01\x00" + pcm + crc16(pcm).to_bytes(2, "little")
        corrupt = bytearray(frame)
        corrupt[30] ^= 0x7f
        parser = PacketParser()
        self.assertEqual(parser.feed(corrupt + frame), [(1, pcm)])
        self.assertEqual(parser.bad_crc, 1)


if __name__ == "__main__":
    unittest.main()
