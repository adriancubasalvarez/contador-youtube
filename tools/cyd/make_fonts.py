"""Genera las fuentes suaves (antialias) de la pantalla CYD en formato VLW,
el que carga LovyanGFX con loadFont(), como arrays de C en
ContadorYouTube/src/cyd/fonts.h.

Usa Fira Sans (licencia OFL, la misma que la versión de tinta).

    python tools/cyd/make_fonts.py

Necesita freetype-py (pip install freetype-py).
"""
import struct
from pathlib import Path

import freetype

ROOT = Path(__file__).resolve().parents[2]
TTF = ROOT / "tools" / "manual" / "fonts"
OUT = ROOT / "ContadorYouTube" / "src" / "cyd" / "fonts.h"

# ASCII + Latin-1 (acentos, ñ, ¿¡) y algunos signos tipográficos habituales
# en los títulos de vídeo
TEXT = [chr(c) for c in range(0x20, 0x7F)] + [chr(c) for c in range(0xA0, 0x100)] + list("—–…·•‘’“”€‹›")
# Cifras grandes: solo lo que producen fmtInt() y fmtCompact()
NUMBERS = list(" +-.,%0123456789KMB—")

FONTS = [
    # nombre,        archivo,                 px, caracteres
    ("FontSmall", "FiraSans-Regular.ttf", 15, TEXT),
    ("FontBody", "FiraSans-Regular.ttf", 19, TEXT),
    ("FontLabel", "FiraSans-SemiBold.ttf", 16, TEXT),
    ("FontStrong", "FiraSans-Bold.ttf", 24, TEXT),
    ("FontValue", "FiraSans-Bold.ttf", 44, NUMBERS),
    ("FontBig", "FiraSans-Bold.ttf", 60, NUMBERS),
    ("FontHero", "FiraSans-Bold.ttf", 76, NUMBERS),
]


def vlw(path, px, chars):
    face = freetype.Face(str(path))
    face.set_pixel_sizes(0, px)
    glyphs = []
    for ch in sorted(set(chars), key=ord):
        if face.get_char_index(ch) == 0:
            continue
        face.load_char(ch, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_LIGHT)
        g = face.glyph
        bm = g.bitmap
        rows = [bytes(bm.buffer[r * bm.pitch : r * bm.pitch + bm.width]) for r in range(bm.rows)]
        glyphs.append((ord(ch), bm.rows, bm.width, (g.advance.x + 32) >> 6, g.bitmap_top, g.bitmap_left, b"".join(rows)))

    ascent = (face.size.ascender + 63) >> 6
    descent = -(face.size.descender >> 6)
    data = struct.pack(">6i", len(glyphs), 11, px, 0, ascent, descent)
    for code, h, w, adv, top, left, _ in glyphs:
        data += struct.pack(">7i", code, h, w, adv, top, left, 0)
    for *_, bitmap in glyphs:
        data += bitmap
    return data


def c_array(name, data):
    lines = [f"static const uint8_t {name}[{len(data)}] PROGMEM = {{"]
    for i in range(0, len(data), 20):
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in data[i : i + 20]) + ",")
    lines.append("};")
    return "\n".join(lines)


def main():
    parts = [
        "// Generado por tools/cyd/make_fonts.py: no editar a mano.",
        "// Fira Sans (SIL Open Font License 1.1, ver src/epd47/fonts/OFL-FiraSans.txt).",
        "#pragma once",
        "#include <Arduino.h>",
        "",
    ]
    total = 0
    for name, ttf, px, chars in FONTS:
        data = vlw(TTF / ttf, px, chars)
        total += len(data)
        parts.append(f"// {ttf} {px} px")
        parts.append(c_array(name, data))
        parts.append("")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(parts), encoding="utf-8")
    print(f"{OUT.relative_to(ROOT)}: {total / 1024:.0f} KB")


if __name__ == "__main__":
    main()
