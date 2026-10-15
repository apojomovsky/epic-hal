#!/usr/bin/env python3
"""Tiny PIC16 (14-bit) disassembler for the XC8 library-routine study.
Decodes program words from an INHX32 hex file. Cycle count: 1 per
instruction, +1 for taken skips (btfsc/btfss/incfsz/decfsz), +1 for
goto/call/return/retlw. Approximate; for algorithm comparison only."""

import sys

F_OPS = {  # bits 11-8 of a byte-oriented op
    0x7: "addwf", 0x5: "andwf", 0x9: "comf", 0x3: "decf",
    0xB: "decfsz", 0xA: "incf", 0xF: "incfsz", 0x4: "iorwf",
    0x8: "movf", 0xD: "rlf", 0xC: "rrf", 0x2: "subwf",
    0xE: "swapf", 0x6: "xorwf",
}
LIT_OPS = {7: "addlw", 6: "sublw", 5: "xorlw", 4: "andlw", 3: "iorlw"}
BIT_OPS = {0: "bcf", 1: "bsf", 2: "btfsc", 3: "btfss"}
SKIP = {"btfsc", "btfss", "incfsz", "decfsz"}


def load_hex(path):
    words = {}
    for line in open(path):
        line = line.strip()
        if not line or line[0] != ":":
            continue
        n = int(line[1:3], 16); addr = int(line[3:7], 16); typ = int(line[7:9], 16)
        data = bytes(int(line[i:i+2], 16) for i in range(9, 9 + 2*n, 2))
        if typ == 0:
            for i in range(0, n, 2):
                words[addr + i // 2] = data[i] | (data[i+1] << 8)
    return words


def disasm_one(w):
    """Return (mnemonic+operands, extra_cycles)."""
    top = (w >> 12) & 3
    if top == 0:
        if w < 0x0200:  # specials: bits 13-8 all zero
            if w == 0x0000: return "nop", 0
            if w == 0x0008: return "return", 1
            if w == 0x0009: return "retfie", 1
            if w == 0x0003: return "sleep", 0
            if w == 0x0004: return "clrwdt", 0
            if w == 0x0002: return "option", 0
            if w >= 0x0180: return f"clrf 0x{w & 0x7F:02X}", 0
            if w >= 0x0100: return "clrw", 0
            if w >= 0x0080: return f"movwf 0x{w & 0x7F:02X}", 0
        m = F_OPS.get((w >> 8) & 0xF)
        if m is None: return f"0x{w:04X}", 0
        d = (w >> 7) & 1
        return f"{m} 0x{w & 0x7F:02X},{'w' if d else 'f'}", 1 if m in SKIP else 0
    if top == 1:
        m = BIT_OPS.get((w >> 10) & 3, "?")
        b = (w >> 7) & 7
        return f"{m} 0x{w & 0x7F:02X},{b}", 1 if m in SKIP else 0
    if top == 2:
        return f"call 0x{w & 0x7FF:03X}", 1
    # top == 3
    if (w >> 11) & 1 == 0:
        return f"goto 0x{w & 0x7FF:03X}", 1
    op = (w >> 8) & 7
    k = w & 0xFF
    if op in (0, 1):
        return f"retlw 0x{k:02X}" if (w >> 9) & 1 else f"movlw 0x{k:02X}", 1 if (w >> 9) & 1 else 0
    m = LIT_OPS.get(op)
    if m is None:
        return f"0x{w:04X}", 0
    return f"{m} 0x{k:02X}", 0


def disasm(words, start, count):
    out = []
    for off in range(count):
        addr = start + off
        w = words.get(addr)
        if w is None:
            out.append((f"{addr:04X}: (unmapped)", 0))
            continue
        s, extra = disasm_one(w)
        out.append((f"{addr:04X}: {w:04X}  {s}", extra))
    return out


def cycles(items):
    return sum(1 + e for _, e in items)


if __name__ == "__main__":
    path, start, count = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3])
    words = load_hex(path)
    items = disasm(words, start, count)
    for line, _ in items:
        print(line)
    print(f"; {count} words, ~{cycles(items)} cycles (branch-neutral, skip-neutral lower bound)")
