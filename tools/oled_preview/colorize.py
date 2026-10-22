"""Convierte los PGM de preview.c en una imagen con los colores del OLED
de dos colores (franja amarilla arriba, azul abajo), ampliada x4."""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

src, out = Path(sys.argv[1]), sys.argv[2]
SCALE, PAD = 4, 24
YELLOW, BLUE, OFF = (255, 214, 10), (70, 170, 255), (12, 14, 18)

frames = []
for pgm in sorted(src.glob("*.pgm")):
    mono = Image.open(pgm).convert("L")
    rgb = Image.new("RGB", mono.size, OFF)
    px, mp = rgb.load(), mono.load()
    for y in range(mono.height):
        for x in range(mono.width):
            if mp[x, y]:
                px[x, y] = YELLOW if y < 16 else BLUE
    big = rgb.resize((mono.width * SCALE, mono.height * SCALE), Image.NEAREST)
    # Separación física entre la zona amarilla y la azul
    ImageDraw.Draw(big).rectangle([0, 16 * SCALE - 2, big.width, 16 * SCALE + 1], fill=OFF)
    frames.append(big)

if len(sys.argv) > 3:  # además, cada pantalla por separado
    frames_dir = Path(sys.argv[3])
    frames_dir.mkdir(parents=True, exist_ok=True)
    for pgm, f in zip(sorted(src.glob("*.pgm")), frames):
        f.save(frames_dir / f"oled-{pgm.stem}.png")

cols = 2
rows = (len(frames) + cols - 1) // cols
fw, fh = frames[0].size
sheet = Image.new("RGB", (cols * (fw + PAD) + PAD, rows * (fh + PAD) + PAD), (40, 40, 44))
for i, f in enumerate(frames):
    sheet.paste(f, (PAD + (i % cols) * (fw + PAD), PAD + (i // cols) * (fh + PAD)))
sheet.save(out)
print(out)
