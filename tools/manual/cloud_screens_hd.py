"""Capturas de Google Cloud en alta resolución para el manual.

Las nuevas (docs/capturas_raw/hd) se hicieron con la página ampliada al 150 %
y guardadas en PNG. Las tres que no se pueden repetir sin crear otro proyecto
u otra clave (notificación, botón Habilitar y clave creada) se amplían x2 a
partir de las originales.

    python tools/manual/cloud_screens_hd.py docs/capturas_raw docs/img
    python tools/manual/cloud_screens_hd.py docs/capturas_raw docs/img-en en     (etiquetas en inglés)
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

src, out = Path(sys.argv[1]), Path(sys.argv[2])
FONT = ImageFont.truetype(str(Path(__file__).parent / "fonts" / "FiraSans-Bold.ttf"), 28)
RED = (217, 43, 43)

# Con "en" como tercer argumento, las etiquetas en inglés (la consola sigue en español)
EN = len(sys.argv) > 3 and sys.argv[3] == "en"
EN_LABELS = {"Pulsa aquí": "Click here", "Crear": "Create", "Habilitar": "Enable", "Copiar": "Copy",
             "Debe poner «Habilitada»": "Must say «Habilitada» (Enabled)"}


def mark(img, box, label=None, at=None, width=6):
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = box
    d.rounded_rectangle([x0 - 6, y0 - 6, x1 + 6, y1 + 6], 14, outline=RED, width=width)
    if label:
        label = EN_LABELS.get(label, label) if EN else label
        lx, ly = at
        w = d.textlength(label, font=FONT)
        d.rounded_rectangle([lx - 12, ly - 8, lx + w + 12, ly + 36], 10, fill=RED)
        d.text((lx, ly - 2), label, font=FONT, fill="white")


hd = {
    "gcloud-selector-proyecto": ("A-bienvenida", [((300, 10, 556, 64), "Pulsa aquí", (590, 22))], None),
    "gc-01-selector": ("B-selector", [((988, 12, 1177, 58), "Pulsa aquí", (720, 20))], None),
    "gc-03-nombre": ("C-nombre", [((60, 285, 828, 337), None, None), ((60, 551, 151, 597), "Crear", (310, 556))],
                     None),
    "gc-06-api-habilitada": ("F-habilitada", [((505, 330, 700, 410), "Debe poner «Habilitada»", (240, 430))],
                             (0, 0, 960, 477)),
    "gc-07-credenciales": ("G-credenciales", [((573, 16, 810, 57), "Pulsa aquí", (590, 72))],
                           (0, 0, 1300, 248)),
    "gc-08-menu-clave": ("H-menu", [((10, 20, 770, 115), None, None)], None),
    "gc-10-restriccion": ("I1-restriccion", [((22, 46, 825, 100), None, None), ((95, 320, 868, 365), None, None)],
                          None),
    "gc-11-crear-clave": ("I2-crear", [((22, 12, 825, 70), None, None), ((28, 535, 160, 568), None, None),
                                       ((22, 797, 112, 843), "Crear", (270, 803))], None),
}
for name, (raw, marks, crop) in hd.items():
    img = Image.open(src / "hd" / f"{raw}.png").convert("RGB")
    for box, label, at in marks:
        mark(img, box, label, at)
    if crop:
        img = img.crop(crop)
    img.save(out / f"{name}.png")

# Las que no se pueden repetir: x2 con un poco de enfoque
for name in ("gc-04-notificacion", "gc-05-api-habilitar", "gc-12-clave-creada"):
    img = Image.open(out / f"{name}.png").convert("RGB")
    if img.width < 700:
        img = img.resize((img.width * 2, img.height * 2), Image.LANCZOS)
        img = img.filter(ImageFilter.UnsharpMask(radius=1.4, percent=90, threshold=2))
        img.save(out / f"{name}.png")
print("ok")
