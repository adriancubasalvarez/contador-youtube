"""Vista previa APROXIMADA del diseño de src/display.cpp, para ajustar el
diseño en el PC sin grabar la placa. Usa las mismas fuentes, tamaños y
coordenadas, pero el dibujo real lo hace la librería de LilyGo.

    python tools/preview.py <carpeta-con-FiraSans-*.ttf> salida.png [setup]
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

W, H = 960, 540
MARGIN, HEADER_H, COL_SPLIT, RIGHT_X, ROW_SPLIT = 32, 72, 530, 564, 338
fonts_dir, out = Path(sys.argv[1]), sys.argv[2]
mode = sys.argv[3] if len(sys.argv) > 3 else "dashboard"


def font(face, pt):  # fontconvert.py rasteriza a 150 ppp
    return ImageFont.truetype(str(fonts_dir / f"FiraSans-{face}.ttf"), round(pt * 150 / 72))


F = {
    "label": font("SemiBold", 9), "body": font("Regular", 12), "title": font("SemiBold", 15),
    "value": font("Bold", 24), "hero": font("Bold", 56),
}
G = lambda t: int(t * 17)  # gris de texto 0..15 -> 0..255

img = Image.new("L", (W, H), 255)
d = ImageDraw.Draw(img)


def text(f, s, x, y, fg=0, right=False):
    d.text((x, y), s, font=F[f], fill=G(fg), anchor="rs" if right else "ls")


def header(title, right):
    d.rectangle([0, 0, W, HEADER_H - 1], fill=0)
    text("title", title, MARGIN, 48, 15)
    text("label", right, W - MARGIN, 46, 12, right=True)


if mode == "setup":
    header("Dashboard YouTube", "Configuración")
    qx, qy, size, scale = MARGIN + 8, HEADER_H + 60, 29, 10
    import random
    random.seed(3)
    for y in range(size):
        for x in range(size):
            finder = any(ox <= x < ox + 7 and oy <= y < oy + 7 for ox, oy in ((0, 0), (22, 0), (0, 22)))
            if (finder and (x in (0, 6, 22, 28) or y in (0, 6, 22, 28) or (2 <= x % 22 <= 4 and 2 <= y % 22 <= 4))) or (
                not finder and random.random() < 0.45):
                d.rectangle([qx + x * scale, qy + y * scale, qx + x * scale + scale - 1, qy + y * scale + scale - 1], fill=0)
    tx, ty = qx + size * scale + 56, HEADER_H + 80
    text("value", "Configúralo en 1 minuto", tx, ty + 30)
    steps = [("1. Escanea el código con la cámara", "del móvil para unirte a su WiFi."),
             ("2. Se abrirá una página: elige tu", "WiFi y escribe tu canal (@micanal)."),
             ("3. Listo. La pantalla se", "actualizará sola.")]
    for i, (a, b) in enumerate(steps):
        y = ty + 104 + i * 92
        text("body", a, tx, y)
        text("body", b, tx, y + 36, 5)
    text("label", "¿No se abre? Conéctate a la WiFi «YT-Dashboard» y entra en http://192.168.4.1",
         MARGIN, H - 28, 8)
else:
    header("Mi Canal de Ejemplo", "16:32 · 87 %")
    text("label", "SUSCRIPTORES", MARGIN, 120, 5)
    text("hero", "128.400", MARGIN - 4, 262)
    text("title", "↑ 1.200", MARGIN, 312)
    text("body", " en 7 días", MARGIN + d.textlength("↑ 1.200", font=F["title"]) + 6, 312, 5)
    text("label", "VISTAS TOTALES", RIGHT_X, 120, 5)
    text("value", "9,9 M", RIGHT_X, 182)
    half = (W - MARGIN - RIGHT_X) // 2
    text("label", "VISTAS HOY", RIGHT_X, 226, 5)
    text("value", "3.412", RIGHT_X, 288)
    text("label", "VÍDEOS", RIGHT_X + half, 226, 5)
    text("value", "214", RIGHT_X + half, 288)
    d.line([COL_SPLIT, HEADER_H + 26, COL_SPLIT, ROW_SPLIT - 14], fill=0xCC)
    d.line([MARGIN, ROW_SPLIT, W - MARGIN, ROW_SPLIT], fill=0xCC)

    x, y, w, h = MARGIN, ROW_SPLIT + 40, COL_SPLIT - 2 * MARGIN, 138
    text("label", "VISTAS POR DÍA · 14 DÍAS", x, y, 5)
    text("label", "máx 4,2 K", x + w, y, 8, right=True)
    vals = [2100, 2600, 1900, 3300, 4186, 3900, 2500, 2200, 2800, 3500, 3100, 2700, 3600, 1400]
    top, bottom, slot = y + 24, y + h, w / 14
    for i, v in enumerate(vals):
        bh = int((bottom - top) * v / 4186)
        bx = x + int(i * slot)
        d.rectangle([bx, bottom - bh, bx + int(slot * 0.7) - 1, bottom], fill=0xCC if i == 13 else 0x44)
    d.line([x, bottom, x + w, bottom], fill=0x88)

    x, y, w = RIGHT_X, ROW_SPLIT + 40, W - MARGIN - RIGHT_X
    text("label", "ÚLTIMO VÍDEO", x, y, 5)
    text("label", "hace 3 días", x + w, y, 8, right=True)
    # mismo ajuste de líneas que wrap() en display.cpp
    title = "Cómo construí un dashboard E-Ink con una Raspberry Pi y un ESP32 paso a paso"
    fits = lambda s: d.textlength(s, font=F["body"]) <= w
    lines, cur = [], ""
    for word in title.split():
        cand = (cur + " " + word).strip()
        if fits(cand) or not cur:
            cur = cand
        elif len(lines) == 1:
            while not fits(cand + "…"):
                cand = cand[:-1]
            cur = cand.rstrip() + "…"
            break
        else:
            lines.append(cur)
            cur = word
    lines.append(cur)
    for i, line in enumerate(lines[:2]):
        text("body", line, x, y + 42 + i * 36)
    value_w = d.textlength("15,3 K", font=F["value"]) + 14
    rest = "vistas · 1,2 K me gusta"
    if d.textlength(rest, font=F["body"]) > w - value_w:
        rest = "vistas"
    text("value", "15,3 K", x, y + 134)
    text("body", rest, x + value_w, y + 134, 5)

img.quantize(16).convert("L").save(out)
print(out)
