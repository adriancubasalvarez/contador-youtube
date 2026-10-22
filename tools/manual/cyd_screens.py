"""Convierte las pantallas de la vista previa de la CYD (los PPM de
tools/cyd_preview) en imágenes para el manual, ampliadas x2 y con un marco
oscuro como el de la pantalla.

    python tools/manual/cyd_screens.py <carpeta-ppm> docs/img
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

src, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
BEZEL, RADIUS, SCALE = 14, 18, 2

for ppm in sorted(src.glob("*.ppm")):
    screen = Image.open(ppm).convert("RGB")
    screen = screen.resize((screen.width * SCALE, screen.height * SCALE), Image.NEAREST)
    w, h = screen.width + 2 * BEZEL, screen.height + 2 * BEZEL
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(img).rounded_rectangle((0, 0, w - 1, h - 1), RADIUS, fill=(22, 22, 26, 255))
    img.paste(screen, (BEZEL, BEZEL))
    name = "cyd-" + ppm.stem + ".png"
    img.save(out / name)
    print(name)
