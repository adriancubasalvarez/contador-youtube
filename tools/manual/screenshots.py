"""Recorta las capturas para el manual (sin datos personales) y añade marcas.

    python tools/manual/screenshots.py <carpeta-capturas> docs/img
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

src, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
FONT = ImageFont.truetype(str(Path(__file__).parent / "fonts" / "FiraSans-Bold.ttf"), 24)
RED = (217, 43, 43)


def mark(img, box, label, label_pos):
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = box
    d.rounded_rectangle([x0 - 6, y0 - 6, x1 + 6, y1 + 6], 14, outline=RED, width=5)
    lx, ly = label_pos
    w = d.textlength(label, font=FONT)
    d.rounded_rectangle([lx - 12, ly - 8, lx + w + 12, ly + 34], 10, fill=RED)
    d.text((lx, ly - 2), label, font=FONT, fill="white")


# Google Cloud: cabecera y bienvenida, sin la foto de perfil ni el número de proyecto
gc = Image.open(src / "5.png").convert("RGB").crop((0, 84, 1000, 372))
mark(gc, (200, 8, 358, 44), "Pulsa aquí", (390, 12))
gc.save(out / "gcloud-selector-proyecto.png")

# Amazon: solo la foto, el nombre y el precio del producto (sin la cabecera con el
# nombre de la cuenta, la dirección de entrega ni el aviso de compra anterior)
Image.open(src / "3.webp").convert("RGB").crop((160, 505, 1365, 1030)).save(out / "compra-esp32.png")
Image.open(src / "4.webp").convert("RGB").crop((160, 488, 1365, 1000)).save(out / "compra-oled.png")
print("ok")
