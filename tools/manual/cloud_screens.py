"""Prepara las capturas de Google Cloud para el manual: tapa los datos
privados (clave, foto de perfil, número de proyecto), marca dónde pulsar y
recorta la zona útil.

    python tools/manual/cloud_screens.py docs/capturas_raw docs/img

Las coordenadas son las de capturas de 1568 px de ancho (la 05 es de 784 px).
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

src, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
FONT = ImageFont.truetype(str(Path(__file__).parent / "fonts" / "FiraSans-Bold.ttf"), 20)
RED = (217, 43, 43)


def blur(img, box):
    region = img.crop(box).filter(ImageFilter.GaussianBlur(9))
    img.paste(region, box)
    ImageDraw.Draw(img).rectangle(box, fill=(225, 228, 232))


def mark(img, box, label=None, at=None):
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = box
    d.rounded_rectangle([x0 - 5, y0 - 5, x1 + 5, y1 + 5], 10, outline=RED, width=4)
    if label:
        lx, ly = at
        w = d.textlength(label, font=FONT)
        d.rounded_rectangle([lx - 10, ly - 6, lx + w + 10, ly + 28], 8, fill=RED)
        d.text((lx, ly - 2), label, font=FONT, fill="white")


PROFILE = (1520, 0, 1568, 42)  # foto de la cuenta, arriba a la derecha

shots = {
    "01-selector.jpg": dict(marks=[((994, 160, 1097, 186), "Pulsa aquí", (840, 196))],
                            crop=(440, 140, 1120, 330)),
    "03-nombre.png": dict(marks=[((30, 157, 440, 186), None, None), ((32, 298, 80, 326), "Crear", (96, 299))],
                          crop=(0, 0, 768, 333)),
    "04-notificacion.jpg": dict(hide=[PROFILE], marks=[((1200, 116, 1316, 136), "Pulsa aquí", (1330, 112))],
                                crop=(1150, 44, 1568, 340)),
    "05-api-habilitar.jpg": dict(hide=[(760, 0, 784, 21)], marks=[((85, 120, 116, 134), "Habilitar", (128, 116))],
                                 crop=(0, 0, 520, 180), scale=2),
    "06-api-habilitada.jpg": dict(marks=[((675, 282, 760, 320), "Debe poner «Habilitada»", (780, 289))],
                                  crop=(225, 150, 1130, 335)),
    "07-credenciales.jpg": dict(marks=[((372, 51, 523, 76), "Pulsa aquí", (540, 52))], crop=(0, 40, 850, 220)),
    "08-menu-clave.jpg": dict(marks=[((378, 88, 785, 136), None, None)], crop=(220, 40, 820, 330)),
    "10-restriccion.jpg": dict(marks=[((1098, 64, 1535, 94), None, None), ((1112, 390, 1525, 414), None, None),
                                      ((1450, 428, 1513, 455), None, None)],
                               crop=(1080, 0, 1568, 470)),
    "11-crear-clave.jpg": dict(marks=[((1098, 225, 1535, 257), None, None), ((1100, 509, 1172, 528), None, None),
                                      ((1097, 652, 1148, 678), "Crear", (1166, 652))],
                               crop=(1080, 200, 1568, 698)),
    "12-clave-creada.jpg": dict(hide=[(1103, 117, 1515, 137)],
                                marks=[((1519, 116, 1541, 138), "Copiar", (1440, 150))],
                                crop=(1080, 0, 1568, 450)),
}

for name, spec in shots.items():
    img = Image.open(src / name).convert("RGB")
    k = spec.get("scale", 1)  # captura de baja resolución: se amplía antes de marcar
    if k != 1:
        img = img.resize((img.width * k, img.height * k), Image.LANCZOS)
    sc = lambda t: tuple(v * k for v in t)
    for box in spec.get("hide", []):
        blur(img, sc(box))
    for box, label, at in spec.get("marks", []):
        mark(img, sc(box), label, at and sc(at))
    img = img.crop(sc(spec["crop"]))
    img.save(out / f"gc-{Path(name).stem}.png")
print("ok")
