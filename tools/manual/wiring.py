"""Dibuja el esquema de conexiones ESP32 <-> OLED para el manual.

Esquema simplificado: solo se dibujan los 4 pines que se usan, alineados
para que cada cable sea una línea recta.

    python tools/manual/wiring.py docs/img/conexiones.png
    python tools/manual/wiring.py docs/img-en/conexiones.png en
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

FONTS = Path(__file__).parent / "fonts"
out = sys.argv[1]
EN = len(sys.argv) > 2 and sys.argv[2] == "en"

S = 2  # escala para que quede nítido en el PDF
W, H = 900 * S, 430 * S
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)


def f(size, weight="Regular"):
    return ImageFont.truetype(str(FONTS / f"FiraSans-{weight}.ttf"), size * S)


def box(x0, y0, x1, y1, fill, r=14):
    d.rounded_rectangle([x0 * S, y0 * S, x1 * S, y1 * S], r * S, fill=fill)


def pin(cx, cy):
    d.rectangle([(cx - 8) * S, (cy - 8) * S, (cx + 8) * S, (cy + 8) * S], fill="#d9b44a")


# Placa ESP32
d.text((210 * S, 34 * S), "ESP32 board" if EN else "Placa ESP32", font=f(20, "SemiBold"), fill="#222", anchor="mm")
box(40, 60, 380, 400, "#1d2a3a")
box(130, 85, 290, 175, "#c9ced6", 6)
d.text((210 * S, 130 * S), "ESP32-WROOM-32", font=f(15, "SemiBold"), fill="#1d2a3a", anchor="mm")
box(175, 380, 245, 412, "#9aa3ad", 6)
d.text((120 * S, 396 * S), "USB-C", font=f(13), fill="#cfd6de", anchor="mm")

# Pantalla OLED (pines dibujados en el lateral para simplificar)
d.text((715 * S, 34 * S), "0.96\" OLED screen" if EN else "Pantalla OLED 0,96\"", font=f(20, "SemiBold"), fill="#222", anchor="mm")
box(560, 60, 870, 400, "#0b2e5c")
box(625, 110, 850, 350, "#05070a", 6)
d.text((737 * S, 140 * S), "SUBSCRIBERS" if EN else "SUSCRIPTORES", font=f(15, "Bold"), fill="#ffd60a", anchor="mm")
d.line([630 * S, 160 * S, 845 * S, 160 * S], fill="#0b2e5c", width=2 * S)
d.text((737 * S, 250 * S), "128,400" if EN else "128.400", font=f(40, "Bold"), fill="#46aaff", anchor="mm")

# (pin ESP32, pin OLED, color del cable, altura)
wires = [
    ("GND", "GND", "#222222", 200),
    ("3V3", "VCC", "#d62828", 250),
    ("GPIO 22", "SCL", "#f2b705", 300),
    ("GPIO 21", "SDA", "#1f7ae0", 350),
]
for esp_label, oled_label, color, y in wires:
    d.line([388 * S, y * S, 552 * S, y * S], fill=color, width=7 * S)
    pin(380, y)
    pin(560, y)
    d.text((364 * S, y * S), esp_label, font=f(18, "SemiBold"), fill="white", anchor="rm")
    d.text((578 * S, y * S), oled_label, font=f(15, "SemiBold"), fill="white", anchor="lm")

img.save(out)
print(out)
