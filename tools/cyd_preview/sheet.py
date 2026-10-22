"""Monta los PPM de preview.cpp en una sola imagen, ampliada x2, con un
marco que imita el borde de la pantalla.

    python sheet.py <carpeta> <salida.png> [patrón, p. ej. "d*.ppm"]"""
import sys
from pathlib import Path

from PIL import Image

src, out = Path(sys.argv[1]), sys.argv[2]
pattern = sys.argv[3] if len(sys.argv) > 3 else "[0-9]*.ppm"
SCALE, PAD, BEZEL, COLS = 2, 28, 10, 2
BG, FRAME = (236, 236, 240), (24, 24, 28)

frames = [Image.open(p).convert("RGB") for p in sorted(src.glob(pattern))]
w, h = frames[0].width * SCALE + 2 * BEZEL, frames[0].height * SCALE + 2 * BEZEL
rows = (len(frames) + COLS - 1) // COLS
sheet = Image.new("RGB", (COLS * w + (COLS + 1) * PAD, rows * h + (rows + 1) * PAD), BG)
for i, f in enumerate(frames):
    x, y = PAD + (i % COLS) * (w + PAD), PAD + (i // COLS) * (h + PAD)
    sheet.paste(FRAME, (x, y, x + w, y + h))
    sheet.paste(f.resize((f.width * SCALE, f.height * SCALE), Image.NEAREST), (x + BEZEL, y + BEZEL))
sheet.save(out)
print(out)
