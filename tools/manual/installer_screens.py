"""Capturas del instalador de Windows para el manual, con datos de ejemplo:
recorre sus pantallas sin placa conectada (no graba nada) y las fotografía.

    python tools/manual/installer_screens.py docs/img
    python tools/manual/installer_screens.py docs/img-en en     (en inglés)
"""
import ctypes
import sys
import time
from pathlib import Path

from PIL import ImageGrab

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "instalador"))
ctypes.windll.shcore.SetProcessDpiAwareness(2)  # coordenadas de Tk = píxeles de la pantalla

import instalador as ins  # noqa: E402

out = Path(sys.argv[1])
EN = len(sys.argv) > 2 and sys.argv[2] == "en"
ins.LANG = "en" if EN else "es"
HOME, CHANNEL = ("MyHome", "My Electronics Channel — 128,400 subscribers") if EN else     ("MiCasa", "Mi Canal de Electrónica — 128.400 suscriptores")
ins.find_ports = lambda: [("COM5", "CH340")]
ins.probe = lambda port: None  # placa nueva, sin el contador
ins.wifi_networks = lambda: [(HOME, False), (HOME + "-5G", True), ("Neighbour_2G" if EN else "Vecino_2G", False)]

app = ins.Installer()
app.attributes("-topmost", True)
app.flash = lambda fw: None          # no graba nada
app.send_config = lambda cfg: None   # ni envía nada


def settle(seconds=0.6):
    end = time.time() + seconds
    while time.time() < end:
        app.update()
        time.sleep(0.02)


def shot(name):
    settle()
    x, y = app.winfo_rootx(), app.winfo_rooty()
    w, h = app.winfo_width(), app.winfo_height()
    ImageGrab.grab(bbox=(x, y, x + w, y + h), all_screens=True).save(out / name)
    print(name)


settle(1)
shot("instalador-1_placa.png")
app.page_connect()
settle(1.5)  # detecta COM5 y "pregunta" a la placa
shot("instalador-2_conectar.png")
app.page_flash(ins.find_firmware("cyd"))
app.on_flash_progress(46)
shot("instalador-3_grabando.png")
app.page_config()
app.password.insert(0, "password" if EN else "contraseña")
app.channel.insert(0, "@mychannel" if EN else "@micanal")
app.key.insert(0, "AIzaSy-EXAMPLE-key-for-the-manual-00000" if EN else "AIzaSy-EJEMPLO-de-clave-para-el-manual0")
app.check_label.config(text="✓ " + CHANNEL, style="Ok.TLabel")
shot("instalador-4_configurar.png")
app.page_sending({"wifi": HOME}, CHANNEL)
app.on_sent("send", True, "")
app.on_sent("wifi", True, "192.168.1.57")
shot("instalador-5_enviando.png")
app.page_done("contador-1a2b.local", "192.168.1.57")
shot("instalador-6_listo.png")
app.destroy()
