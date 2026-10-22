"""Estilos y utilidades comunes de los manuales (build_manual.py en español y
build_manual_en.py en inglés). TEXT guarda las pocas palabras fijas de cada idioma.
"""
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import cm, mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    CondPageBreak, Flowable, Image, KeepTogether, PageBreak, Paragraph, Preformatted,
    SimpleDocTemplate, Spacer, Table, TableStyle,
)

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent.parent

# Palabras fijas de los recuadros, los pasos y el pie de página: build_manual_en.py las cambia
TEXT = {"tip": "Consejo", "warn": "Atención", "photo": "Para el Instructable: fotos de este paso", "step": "PASO",
        "footer": "Contador de YouTube de escritorio · manual de montaje y uso"}
DOCS = PROJECT / "docs"

for name, file in [("Fira", "FiraSans-Regular"), ("Fira-Semi", "FiraSans-SemiBold"),
                   ("Fira-Bold", "FiraSans-Bold"), ("Mono", "FiraMono-Regular")]:
    pdfmetrics.registerFont(TTFont(name, str(HERE / "fonts" / f"{file}.ttf")))
pdfmetrics.registerFontFamily("Fira", normal="Fira", bold="Fira-Bold", italic="Fira", boldItalic="Fira-Bold")

RED = colors.HexColor("#d92b2b")
INK = colors.HexColor("#1b1b1b")
MUTED = colors.HexColor("#6b6b6b")
LINE = colors.HexColor("#e2e2de")
TIP_BG = colors.HexColor("#fff6d6")
WARN_BG = colors.HexColor("#fde8e8")
PHOTO_BG = colors.HexColor("#e6f1fd")
CODE_BG = colors.HexColor("#f2f2ef")

PAGE_W, PAGE_H = A4
MARGIN = 2 * cm
CONTENT_W = PAGE_W - 2 * MARGIN

st = {
    "body": ParagraphStyle("body", fontName="Fira", fontSize=10.5, leading=15.5, textColor=INK, spaceAfter=6),
    "small": ParagraphStyle("small", fontName="Fira", fontSize=9, leading=12.5, textColor=MUTED),
    "cell": ParagraphStyle("cell", fontName="Fira", fontSize=9.5, leading=13, textColor=INK),
    "cellb": ParagraphStyle("cellb", fontName="Fira-Semi", fontSize=9.5, leading=13, textColor=INK),
    "h1": ParagraphStyle("h1", fontName="Fira-Bold", fontSize=22, leading=27, textColor=INK, spaceBefore=4,
                         spaceAfter=10),
    "h2": ParagraphStyle("h2", fontName="Fira-Bold", fontSize=13.5, leading=18, textColor=INK, spaceBefore=12,
                         spaceAfter=6, keepWithNext=1),
    "kicker": ParagraphStyle("kicker", fontName="Fira-Semi", fontSize=10, leading=13, textColor=RED),
    "step": ParagraphStyle("step", fontName="Fira", fontSize=10.5, leading=15, textColor=INK, leftIndent=18,
                           firstLineIndent=-18, spaceAfter=5),
    "cover_title": ParagraphStyle("ct", fontName="Fira-Bold", fontSize=34, leading=40, textColor=INK),
    "cover_sub": ParagraphStyle("cs", fontName="Fira", fontSize=15, leading=21, textColor=MUTED),
    "caption": ParagraphStyle("cap", fontName="Fira", fontSize=8.5, leading=11, textColor=MUTED,
                              alignment=TA_CENTER, spaceBefore=3, spaceAfter=10),
}


# ------------------------------------------------------------- utilidades ---

def P(text, style="body"):
    return Paragraph(text, st[style])


def code(s):
    return f'<font name="Mono" size="9.5">{s}</font>'


def steps(*items, start=1):
    return [P(f"<b>{i}.</b>&nbsp;&nbsp;{t}", "step") for i, t in enumerate(items, start)]


def bullets(*items):
    return [P(f"•&nbsp;&nbsp;{t}", "step") for t in items]


def box(title, items, bg, accent):
    content = [P(f"<b>{title}</b>", "cellb")] + [P(t, "cell") for t in items]
    t = Table([[content]], colWidths=[CONTENT_W])
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), bg),
        ("LINEBEFORE", (0, 0), (0, -1), 3, accent),
        ("LEFTPADDING", (0, 0), (-1, -1), 12), ("RIGHTPADDING", (0, 0), (-1, -1), 12),
        ("TOPPADDING", (0, 0), (-1, -1), 8), ("BOTTOMPADDING", (0, 0), (-1, -1), 9),
    ]))
    return KeepTogether([Spacer(1, 4), t, Spacer(1, 8)])


def tip(*items):
    return box(TEXT["tip"], items, TIP_BG, colors.HexColor("#e0a800"))


def warn(*items):
    return box(TEXT["warn"], items, WARN_BG, RED)


def photo(*items):
    return box(TEXT["photo"], items, PHOTO_BG, colors.HexColor("#1f7ae0"))


def codeblock(text):
    pre = Preformatted(text, ParagraphStyle("code", fontName="Mono", fontSize=9.5, leading=13.5, textColor=INK))
    t = Table([[pre]], colWidths=[CONTENT_W])
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), CODE_BG),
        ("LEFTPADDING", (0, 0), (-1, -1), 12), ("TOPPADDING", (0, 0), (-1, -1), 8),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 8),
    ]))
    return KeepTogether([Spacer(1, 2), t, Spacer(1, 8)])


def table(rows, widths, header=True, style="cell"):
    data = [[P(c, "cellb" if header and r == 0 else style) if isinstance(c, str) else c for c in row]
            for r, row in enumerate(rows)]
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    cmds = [
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LINEBELOW", (0, 0), (-1, -1), 0.5, LINE),
        ("LEFTPADDING", (0, 0), (-1, -1), 6), ("RIGHTPADDING", (0, 0), (-1, -1), 6),
        ("TOPPADDING", (0, 0), (-1, -1), 5), ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
    ]
    if header:
        cmds += [("LINEBELOW", (0, 0), (-1, 0), 1.2, INK)]
    t.setStyle(TableStyle(cmds))
    return t


def picture(name, width):
    img = Image(str(DOCS / name))
    img.drawWidth, img.drawHeight = width, width * img.imageHeight / img.imageWidth
    return img


def image(name, width, caption=None):
    items = [picture(name, width)]
    if caption:
        items.append(P(caption, "caption"))
    return KeepTogether(items)


def step_title(number, title, time=None):
    kicker = f"{TEXT['step']} {number}" + (f"  ·  {time}" if time else "")
    return [CondPageBreak(9 * cm), P(kicker, "kicker"), P(title, "h1")]


class CheckBox(Flowable):
    def __init__(self, size=10):
        super().__init__()
        self.size = size
        self.width = self.height = size

    def draw(self):
        self.canv.setStrokeColor(INK)
        self.canv.setLineWidth(0.8)
        self.canv.rect(0, 0, self.size, self.size)


def on_page(canvas, doc):
    if doc.page == 1:
        return
    canvas.saveState()
    canvas.setFont("Fira", 8.5)
    canvas.setFillColor(MUTED)
    canvas.drawString(MARGIN, 1.2 * cm, TEXT["footer"])
    canvas.drawRightString(PAGE_W - MARGIN, 1.2 * cm, str(doc.page))
    canvas.setStrokeColor(LINE)
    canvas.line(MARGIN, 1.55 * cm, PAGE_W - MARGIN, 1.55 * cm)
    canvas.restoreState()


def grid(names, width, cols=2, captions=None):
    """Varias imágenes en rejilla, con pie opcional bajo cada una."""
    cells = []
    for i, n in enumerate(names):
        cell = [picture(n, width)]
        if captions:
            cell.append(P(captions[i], "caption"))
        cells.append(cell)
    rows = [cells[i:i + cols] for i in range(0, len(cells), cols)]
    if len(rows[-1]) < cols:
        rows[-1] += [""] * (cols - len(rows[-1]))
    t = Table(rows, colWidths=[CONTENT_W / cols] * cols)
    t.setStyle(TableStyle([("ALIGN", (0, 0), (-1, -1), "CENTER"), ("VALIGN", (0, 0), (-1, -1), "TOP"),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]))
    return t


def side(img_name, img_width, flowables):
    """Imagen a la izquierda y texto a la derecha."""
    t = Table([[picture(img_name, img_width), flowables]], colWidths=[img_width + 0.6 * cm, CONTENT_W - img_width - 0.6 * cm])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0)]))
    return KeepTogether([t, Spacer(1, 6)])


def badge(text):
    return f'<font name="Fira-Semi" color="#d92b2b">{text}</font>'


