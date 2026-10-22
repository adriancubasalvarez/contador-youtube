"""Instalador del Contador YouTube para Windows.

Paso a paso: elegir la placa, conectarla por USB, grabar el firmware y enviar
por el mismo cable la WiFi, el canal, la clave de API (comprobados antes
contra YouTube desde el PC) y el idioma. No hace falta Arduino IDE ni el móvil.
En español o en inglés: según el idioma de Windows, y se puede cambiar.

Desarrollo:  python tools/instalador/instalador.py   (usa los .bin de dist/)
Ejecutable:  python tools/instalador/build_exe.py    (dist/InstaladorContadorYouTube.exe)

Copyright (c) 2026 Adrian Cubas · GPL-2.0-or-later (ver LICENSE en esta carpeta).
"""
import contextlib
import ctypes
import json
import queue
import re
import subprocess
import sys
import threading
import time
import tkinter as tk
import urllib.error
import urllib.parse
import urllib.request
import webbrowser
from pathlib import Path
from tkinter import ttk

import serial
from serial.tools import list_ports

HERE = Path(__file__).resolve().parent
BUNDLE = Path(getattr(sys, "_MEIPASS", HERE))  # dentro del .exe, los archivos van aquí
IMG = BUNDLE / "img"
FIRMWARE_DIRS = [BUNDLE / "firmware", HERE.parents[1] / "dist"]

# Idioma de las ventanas (y, por defecto, del contador): "es" o "en"
def windows_language():
    try:
        return "es" if ctypes.windll.kernel32.GetUserDefaultUILanguage() & 0x3FF == 0x0A else "en"
    except (AttributeError, OSError):
        return "es"


LANG = windows_language()


def t(es, en):
    return en if LANG == "en" else es


def boards():
    return {
        "cyd": (t("Pantalla táctil a color", "Color touch screen"),
                t("Placa de 2,8\" con el ESP32 integrado\n(Freenove «Cheap Yellow Display»)",
                  "2.8\" board with a built-in ESP32\n(Freenove \"Cheap Yellow Display\")"), t("cyd.png", "cyd_en.png")),
        "oled": (t("Contador OLED", "OLED counter"),
                 t("ESP32 + pantalla OLED de 0,96\"\n(cuatro cables)", "ESP32 + 0.96\" OLED screen\n(four wires)"),
                 t("oled.png", "oled_en.png")),
    }


LANGUAGES = {"es": "Español", "en": "English"}
USB_CHIPS = {0x1A86: "CH340", 0x10C4: "CP210x", 0x303A: "ESP32"}
KEY_RE = re.compile(r"^AIza[0-9A-Za-z_\-]{35}$")
CH340_DRIVER = "https://www.wch-ic.com/downloads/CH341SER_EXE.html"
KEY_HELP = "https://console.cloud.google.com/apis/library/youtube.googleapis.com"
NO_WINDOW = 0x08000000  # que netsh no abra una consola


# --- Firmware y placa --------------------------------------------------------

def find_firmware(board):
    """El -completo.bin de esa placa con la versión más alta."""
    found = []
    for d in FIRMWARE_DIRS:
        for f in d.glob(f"contador-{board}-v*-completo.bin"):
            m = re.search(r"-v(\d+)\.(\d+)\.(\d+)-completo", f.name)
            if m:
                found.append((tuple(map(int, m.groups())), f))
    return max(found)[1] if found else None


def version_of(path):
    m = re.search(r"-v([\d.]+)-completo", path.name)
    return m.group(1) if m else "?"


def find_ports():
    return [(p.device, USB_CHIPS[p.vid]) for p in list_ports.comports() if p.vid in USB_CHIPS]


def open_and_reset(port):
    """Abre el puerto y reinicia la placa (EN por RTS) sin entrar en modo de carga."""
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 0.1
    s.dtr = False
    s.rts = False
    s.open()
    s.rts = True
    time.sleep(0.1)
    s.rts = False
    return s


def read_until(s, pattern, seconds, log=None):
    """Lee líneas hasta que una cumpla 'pattern' (regex). Devuelve el match o None."""
    end, buf = time.time() + seconds, b""
    while time.time() < end:
        buf += s.read(512)
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            text = line.decode("utf-8", "replace").strip()
            if log and text:
                log(text)
            m = re.search(pattern, text)
            if m:
                return m
    return None


def probe(port):
    """Pregunta a la placa qué tiene instalado. None si no lleva el contador."""
    try:
        with contextlib.closing(open_and_reset(port)) as s:
            if not read_until(s, r"CONTADOR-YT LISTO", 4):
                return None
            s.write(b"INFO\n")
            m = read_until(s, r"^INFO (\{.*\})$", 2)
            return json.loads(m.group(1)) if m else None
    except (serial.SerialException, ValueError):
        return None


# --- WiFi del PC --------------------------------------------------------------

def wifi_networks():
    """Redes que ve este PC: [(nombre, solo_5ghz)], la conectada primero."""
    def netsh(*args):
        try:
            r = subprocess.run(["netsh", "wlan", *args], capture_output=True, creationflags=NO_WINDOW)
            return r.stdout.decode("cp850", "replace")
        except OSError:
            return ""

    current = None
    for line in netsh("show", "interfaces").splitlines():
        m = re.match(r"^\s+SSID\s+:\s(.+)$", line)
        if m:
            current = m.group(1).strip()
    bands, name = {}, None
    for line in netsh("show", "networks", "mode=bssid").splitlines():
        m = re.match(r"^SSID \d+\s*:\s?(.*)$", line)
        if m:
            name = m.group(1).strip()
            if name:
                bands.setdefault(name, set())
        elif name and "GHz" in line:
            bands[name].add("5" if re.search(r"\b5(\.\d+)?\s*GHz", line) else "2.4")
    # Las redes que este PC tiene guardadas (puede que la de 2,4 GHz esté ahí)
    for line in netsh("show", "profiles").splitlines():
        m = re.match(r"^\s+(?:Perfil|All User Profile|User Profile)[^:]*:\s(.+)$", line)
        if m:
            bands.setdefault(m.group(1).strip(), set())
    # Muchos routers separan la red de 5 GHz con otro nombre ("...-PLUS", "..._5G"):
    # se propone la de 2,4 GHz quitando ese añadido
    for n in list(bands):
        if bands[n] == {"5"}:
            twin = re.sub(r"[-_ ]?(PLUS|5G|5GHZ|5 GHZ|5)(?=[-_ ]|$)", "", n, count=1, flags=re.IGNORECASE).strip("-_ ")
            if twin and twin != n:
                bands.setdefault(twin, set())
    if current:
        bands.setdefault(current, set())
    bands.pop("YT-Dashboard", None)  # la red de configuración del propio contador
    nets = [(n, b == {"5"}) for n, b in bands.items()]
    nets.sort(key=lambda x: (x[1], x[0] != current, x[0].lower()))  # las que sirven primero
    return nets


# --- Comprobar canal y clave contra YouTube ------------------------------------

def channel_selector(text):
    """Igual que el firmware: @canal, URL o ID -> parámetro de channels.list."""
    t = text.strip()
    m = re.search(r"youtube\.com/(.+)", t)
    if m:
        t = m.group(1).split("?")[0]
        if t.startswith("channel/"):
            return "id=" + t[8:].split("/")[0]
        if t.startswith("user/"):
            return "forUsername=" + urllib.parse.quote(t[5:].split("/")[0])
        if t.startswith("c/"):
            t = t[2:]
        t = t.split("/")[0]
    if len(t) == 24 and t.startswith("UC"):
        return "id=" + t
    if not t.startswith("@"):
        t = "@" + t
    return "forHandle=" + urllib.parse.quote(t)


def check_channel(channel, key):
    """(True, "Nombre — 3.000 suscriptores") o (False, explicación)."""
    if not KEY_RE.match(key):
        return False, t("La clave debe empezar por AIza y tener 39 caracteres. Cópiala entera, sin espacios.",
                        "The key must start with AIza and be 39 characters long. Copy all of it, with no spaces.")
    if not channel.strip():
        return False, t("Escribe tu canal: el @nombre que sale en YouTube, o el enlace del canal.",
                        "Type your channel: the @handle shown on YouTube, or the channel link.")
    url = ("https://www.googleapis.com/youtube/v3/channels?part=snippet,statistics&"
           + channel_selector(channel)
           + "&fields=items(snippet/title,statistics(subscriberCount,hiddenSubscriberCount))&key="
           + urllib.parse.quote(key))
    try:
        with urllib.request.urlopen(url, timeout=15) as r:
            data = json.load(r)
    except urllib.error.HTTPError as e:
        try:
            err = json.load(e)["error"]
            reason = err["errors"][0].get("reason", "")
            message = err.get("message", "")
        except Exception:
            reason, message = "", ""
        if reason in ("quotaExceeded", "dailyLimitExceeded"):
            return False, t("Esta clave ha agotado su cuota de hoy. Vuelve a intentarlo mañana.",
                            "This key has used up today's quota. Try again tomorrow.")
        if reason == "accessNotConfigured" or "has not been used" in message or "disabled" in message:
            return False, t("La clave existe, pero no tiene activada la «YouTube Data API v3». Actívala en "
                            "Google Cloud: APIs y servicios → Biblioteca.",
                            "The key exists, but \"YouTube Data API v3\" is not enabled for it. Enable it in "
                            "Google Cloud: APIs & Services → Library.")
        if "API key not valid" in message or reason == "keyInvalid":
            return False, t("Google dice que la clave no es válida. Cópiala de nuevo.",
                            "Google says the key is not valid. Copy it again.")
        return False, t(f"YouTube ha rechazado la clave ({e.code}). {message}", f"YouTube rejected the key ({e.code}). {message}")
    except (urllib.error.URLError, OSError):
        return False, t("Este ordenador no tiene conexión a Internet: no se puede comprobar.",
                        "This computer has no Internet connection: it cannot be checked.")
    items = data.get("items") or []
    if not items:
        return False, t(f"No se encuentra el canal «{channel.strip()}». Escribe el @nombre tal como sale en YouTube.",
                        f"Channel \"{channel.strip()}\" not found. Type the @handle exactly as shown on YouTube.")
    title = items[0]["snippet"]["title"]
    stats = items[0].get("statistics", {})
    if stats.get("hiddenSubscriberCount"):
        return True, t(f"{title} — suscriptores ocultos", f"{title} — subscriber count hidden")
    subs = int(stats.get("subscriberCount", 0))
    return True, t(f"{title} — {subs:,} suscriptores".replace(",", "."), f"{title} — {subs:,} subscribers")


# --- Interfaz -----------------------------------------------------------------

class Installer(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(t("Instalador del Contador YouTube", "YouTube Counter installer"))
        self.geometry("660x560")
        self.resizable(False, False)
        try:
            self.iconbitmap(IMG / "icono.ico")
        except tk.TclError:
            pass
        self.events = queue.Queue()  # mensajes de los hilos de trabajo
        self.board = tk.StringVar(value="cyd")
        self.counter_lang = LANG  # idioma que se pondrá en el contador
        self.port = None
        self.info = None      # lo que dice el contador conectado (si ya lo tiene instalado)
        self.defaults = {}    # su WiFi y canal: se proponen aunque se reinstale
        self.images = {}

        style = ttk.Style(self)
        style.configure("Title.TLabel", font=("Segoe UI Semibold", 15))
        style.configure("Big.TButton", font=("Segoe UI", 11), padding=8)
        style.configure("Muted.TLabel", foreground="#666")
        style.configure("Ok.TLabel", foreground="#1f7a3a")
        style.configure("Err.TLabel", foreground="#b3261e")

        head = ttk.Frame(self, padding=(16, 12))
        head.pack(fill="x")
        logo = self.image("logo.png", 40)
        if logo:
            ttk.Label(head, image=logo).pack(side="left", padx=(0, 10))
        self.head_title = ttk.Label(head, text=t("Contador YouTube", "YouTube Counter"), style="Title.TLabel")
        self.head_title.pack(side="left")
        self.step_label = ttk.Label(head, style="Muted.TLabel")
        self.step_label.pack(side="right")
        ttk.Separator(self).pack(fill="x")

        self.body = ttk.Frame(self, padding=20)
        self.body.pack(fill="both", expand=True)
        self.after(100, self.poll_events)
        self.page_board()

    # utilidades
    def image(self, name, height=None):
        path = IMG / name
        if not path.exists():
            return None
        img = tk.PhotoImage(file=str(path))
        if height and img.height() > height:
            img = img.subsample(max(1, img.height() // height))
        self.images[name] = img
        return img

    def clear(self, step, title):
        for w in self.body.winfo_children():
            w.destroy()
        self.step_label.config(text=step)
        ttk.Label(self.body, text=title, style="Title.TLabel").pack(anchor="w", pady=(0, 12))

    def text(self, parent, msg, style=None, wrap=600, **pack):
        lbl = ttk.Label(parent, text=msg, wraplength=wrap, justify="left", style=style or "TLabel")
        lbl.pack(anchor="w", **pack)
        return lbl

    def worker(self, fn, *args):
        threading.Thread(target=fn, args=args, daemon=True).start()

    def post(self, *event):
        self.events.put(event)

    def poll_events(self):
        try:
            while True:
                kind, *args = self.events.get_nowait()
                getattr(self, "on_" + kind)(*args)
        except queue.Empty:
            pass
        self.after(100, self.poll_events)

    # 1. Idioma y placa
    def set_language(self, lang):
        global LANG
        LANG = self.counter_lang = lang
        self.title(t("Instalador del Contador YouTube", "YouTube Counter installer"))
        self.head_title.config(text=t("Contador YouTube", "YouTube Counter"))
        self.page_board()

    def page_board(self):
        self.clear(t("Paso 1 de 4", "Step 1 of 4"), t("¿Qué contador tienes?", "Which counter do you have?"))
        langs = ttk.Frame(self.body)
        langs.place(relx=1.0, y=4, anchor="ne")
        ttk.Label(langs, text="Idioma · Language:", style="Muted.TLabel").pack(side="left", padx=(0, 6))
        self.lang_var = tk.StringVar(value=LANG)  # en self: si no, Tk la pierde y no marca ninguna
        for code, name in LANGUAGES.items():
            ttk.Radiobutton(langs, text=name, value=code, variable=self.lang_var,
                            command=lambda c=code: self.set_language(c)).pack(side="left")
        cards = ttk.Frame(self.body)
        cards.pack(fill="x", pady=8)
        for col, (key, (name, desc, img)) in enumerate(boards().items()):
            card = ttk.Frame(cards, padding=10, relief="groove")
            card.grid(row=0, column=col, padx=8, sticky="n")
            pic = self.image(img)
            ttk.Radiobutton(card, text=name, value=key, variable=self.board).pack(anchor="w")
            if pic:
                lbl = ttk.Label(card, image=pic)
                lbl.pack(pady=6)
                lbl.bind("<Button-1>", lambda e, k=key: self.board.set(k))
            ttk.Label(card, text=desc, style="Muted.TLabel", justify="left").pack(anchor="w")
        ttk.Button(self.body, text=t("Siguiente  ›", "Next  ›"), style="Big.TButton", command=self.page_connect).pack(
            side="bottom", anchor="e")

    # 2. Conectar
    def page_connect(self):
        self.clear(t("Paso 2 de 4", "Step 2 of 4"), t("Conecta el contador al ordenador", "Connect the counter to the computer"))
        self.text(self.body, t("Usa un cable USB que sirva para datos (algunos cables de carga no valen). "
                               "Esta ventana lo detecta sola.",
                               "Use a USB cable that carries data (some charging-only cables do not work). "
                               "This window detects it by itself."))
        self.connect_status = self.text(self.body, t("Buscando…", "Searching…"), pady=(16, 4))
        self.connect_detail = self.text(self.body, "", style="Muted.TLabel")
        self.connect_buttons = ttk.Frame(self.body)
        self.connect_buttons.pack(side="bottom", fill="x")
        ttk.Button(self.connect_buttons, text=t("‹  Atrás", "‹  Back"), command=self.page_board).pack(side="left")
        self.port = None
        self.searching_since = time.time()
        self.scan_ports()

    def scan_ports(self):
        if not self.connect_status.winfo_exists() or self.port:
            return
        ports = find_ports()
        if ports:
            self.port, chip = ports[0]
            self.connect_status.config(text=t(f"Placa encontrada en {self.port} ({chip}). Comprobando qué tiene…",
                                              f"Board found on {self.port} ({chip}). Checking what it has…"))
            self.worker(lambda p=self.port: self.post("probed", probe(p)))
            return
        if time.time() - self.searching_since > 8:
            self.connect_status.config(text=t("Todavía no aparece ninguna placa.", "No board shows up yet."))
            self.connect_detail.config(text=t(
                "• Prueba otro cable o otro puerto USB.\n"
                "• Si es la primera vez, Windows puede necesitar el controlador CH340: "
                "pulsa el botón de abajo, instálalo y vuelve a conectar la placa.",
                "• Try another cable or another USB port.\n"
                "• The first time, Windows may need the CH340 driver: "
                "click the button below, install it and plug the board in again."))
            if not getattr(self, "driver_button", None) or not self.driver_button.winfo_exists():
                self.driver_button = ttk.Button(self.connect_buttons, text=t("Descargar el controlador CH340",
                                                                             "Download the CH340 driver"),
                                                command=lambda: webbrowser.open(CH340_DRIVER))
                self.driver_button.pack(side="right")
        self.after(1000, self.scan_ports)

    def on_probed(self, info):
        if not self.connect_status.winfo_exists():
            return
        self.info = info
        if info:
            self.defaults = {"wifi": info.get("wifi", ""), "canal": info.get("canal", "")}
        board = self.board.get()
        fw = find_firmware(board)
        for w in self.connect_buttons.winfo_children():
            w.destroy()
        ttk.Button(self.connect_buttons, text=t("‹  Atrás", "‹  Back"), command=self.page_board).pack(side="left")
        if not fw:
            self.connect_status.config(text=t("No se encuentra el firmware para esta placa.",
                                              "The firmware for this board is missing."), style="Err.TLabel")
            return
        if info:
            self.connect_status.config(text=t(f"En {self.port} hay un contador instalado (versión {info.get('version')}, "
                                              f"placa «{info.get('placa')}»).",
                                              f"There is a counter installed on {self.port} (version {info.get('version')}, "
                                              f"board \"{info.get('placa')}\")."))
            if info.get("placa") != board:
                self.connect_detail.config(text=t("Ojo: es de la otra placa. Vuelve atrás si te equivocaste al elegir.",
                                                  "Careful: it is the other board. Go back if you picked the wrong one."),
                                           style="Err.TLabel")
            else:
                ttk.Button(self.connect_buttons, text=t("Solo cambiar la configuración", "Only change the settings"),
                           command=self.page_config).pack(side="right", padx=(8, 0))
            label = t(f"Reinstalar (v{version_of(fw)})", f"Reinstall (v{version_of(fw)})")
        else:
            self.connect_status.config(text=t(f"Placa lista en {self.port}.", f"Board ready on {self.port}."))
            self.connect_detail.config(text=t("Se instalará el contador en ella. Lo que tuviera antes se borra.",
                                              "The counter will be installed on it. Whatever it had before is erased."))
            label = t("Instalar el contador", "Install the counter")
        ttk.Button(self.connect_buttons, text=label + "  ›", style="Big.TButton",
                   command=lambda: self.page_flash(fw)).pack(side="right")

    # 3. Grabar
    def page_flash(self, fw):
        self.clear(t("Paso 3 de 4", "Step 3 of 4"), t("Instalando el contador", "Installing the counter"))
        self.text(self.body, t(f"Firmware {version_of(fw)} para «{boards()[self.board.get()][0]}». "
                               "Tarda menos de un minuto. No desconectes el cable.",
                               f"Firmware {version_of(fw)} for \"{boards()[self.board.get()][0]}\". "
                               "It takes less than a minute. Do not unplug the cable."))
        self.progress = ttk.Progressbar(self.body, length=600, maximum=100)
        self.progress.pack(pady=(24, 6))
        self.flash_status = self.text(self.body, t("Conectando con la placa…", "Connecting to the board…"), style="Muted.TLabel")
        self.worker(self.flash, fw)

    def flash(self, fw):
        import esptool

        class Progress:  # recoge la salida de esptool y saca el porcentaje
            def __init__(self, post):
                self.post, self.buf = post, ""

            def write(self, s):
                self.buf = (self.buf + s)[-400:]
                pcts = re.findall(r"(\d{1,3}(?:\.\d+)?)\s?%", self.buf)
                if pcts:
                    self.post("flash_progress", float(pcts[-1]))
                return len(s)

            def flush(self):
                pass

        error = None
        for baud in ("921600", "460800", "115200"):  # algunos cables o clones no aguantan la máxima
            out = Progress(self.post)
            try:
                with contextlib.redirect_stdout(out), contextlib.redirect_stderr(out):
                    esptool.main(["--chip", "esp32", "--port", self.port, "--baud", baud,
                                  "write-flash", "0x0", str(fw)])
                error = None
                break
            except SystemExit as e:
                error = None if not e.code else out.buf[-300:]
                if not error:
                    break
            except Exception as e:  # noqa: BLE001 - se muestra al usuario
                error = f"{e}\n{out.buf[-300:]}"
            self.post("flash_retry", baud)
            time.sleep(1)
        self.post("flashed", error)

    def on_flash_progress(self, pct):
        if self.progress.winfo_exists():
            self.progress["value"] = pct
            self.flash_status.config(text=t(f"Grabando… {pct:.0f} %", f"Writing… {pct:.0f} %"))

    def on_flash_retry(self, baud):
        if self.progress.winfo_exists():
            self.flash_status.config(text=t(f"No fue bien a {baud} baudios. Reintentando más despacio…",
                                            f"It did not work at {baud} baud. Retrying slower…"))

    def on_flashed(self, error):
        if error:
            self.clear(t("Paso 3 de 4", "Step 3 of 4"), t("No se pudo instalar", "Could not install"))
            self.text(self.body, t("Prueba: otro cable o puerto USB; si no, mantén pulsado el botón BOOT de la placa "
                                   "mientras pulsas «Reintentar».",
                                   "Try another cable or USB port; otherwise, hold the BOOT button on the board "
                                   "while you click \"Retry\"."))
            self.text(self.body, error.strip()[-400:], style="Muted.TLabel", pady=12)
            row = ttk.Frame(self.body)
            row.pack(side="bottom", fill="x")
            ttk.Button(row, text=t("‹  Atrás", "‹  Back"), command=self.page_connect).pack(side="left")
            fw = find_firmware(self.board.get())
            ttk.Button(row, text=t("Reintentar", "Retry"), style="Big.TButton",
                       command=lambda: self.page_flash(fw)).pack(side="right")
            return
        self.info = None
        self.page_config()

    # 4. Configurar
    def page_config(self):
        self.clear(t("Paso 4 de 4", "Step 4 of 4"), t("Tu WiFi y tu canal", "Your WiFi and your channel"))
        form = ttk.Frame(self.body)
        form.pack(fill="x")
        form.columnconfigure(1, weight=1)

        def row(r, label, widget, note=None):
            ttk.Label(form, text=label).grid(row=r, column=0, sticky="w", pady=4, padx=(0, 10))
            widget.grid(row=r, column=1, sticky="ew", pady=4)
            if note:
                ttk.Label(form, text=note, style="Muted.TLabel").grid(row=r + 1, column=1, sticky="w")

        self.nets = wifi_networks()
        self.five_note = t("   (solo 5 GHz: no sirve)", "   (5 GHz only: will not work)")
        self.ssid = ttk.Combobox(form, values=[n + (self.five_note if five else "") for n, five in self.nets])
        if self.defaults.get("wifi"):
            self.ssid.set(self.defaults["wifi"])
        elif self.nets:
            self.ssid.set(self.nets[0][0])
        row(0, t("Red WiFi", "WiFi network"), self.ssid,
            t("Tiene que ser de 2,4 GHz. Puedes escribir el nombre si no aparece.",
              "It must be 2.4 GHz. You can type the name if it is not listed."))
        pw_box = ttk.Frame(form)
        self.password = ttk.Entry(pw_box, show="•")
        self.password.pack(side="left", fill="x", expand=True)
        self.show_pw = tk.BooleanVar()
        ttk.Checkbutton(pw_box, text=t("ver", "show"), variable=self.show_pw,
                        command=lambda: self.password.config(show="" if self.show_pw.get() else "•")).pack(side="left")
        row(2, t("Contraseña", "Password"), pw_box)
        self.channel = ttk.Entry(form)
        if self.defaults.get("canal"):
            self.channel.insert(0, self.defaults["canal"])
        row(3, t("Tu canal", "Your channel"), self.channel,
            t("El @nombre de YouTube o el enlace del canal.", "Your YouTube @handle or the channel link."))
        self.key = ttk.Entry(form)
        row(5, t("Clave de API", "API key"), self.key)
        link = ttk.Label(form, text=t("¿Cómo consigo mi clave? (gratis, 5 minutos)", "How do I get my key? (free, 5 minutes)"),
                         foreground="#1a5fb4", cursor="hand2")
        link.grid(row=6, column=1, sticky="w")
        link.bind("<Button-1>", lambda e: self.show_key_help())
        if self.info and self.info.get("clave"):
            ttk.Label(form, text=t("El contador ya tiene una clave: déjalo vacío para mantenerla.",
                                   "The counter already has a key: leave it empty to keep it."),
                      style="Muted.TLabel").grid(row=7, column=1, sticky="w")
        self.lang_box = ttk.Combobox(form, state="readonly", values=list(LANGUAGES.values()), width=18)
        self.lang_box.set(LANGUAGES[self.counter_lang])
        ttk.Label(form, text=t("Idioma del contador", "Counter language")).grid(row=8, column=0, sticky="w", pady=(8, 4),
                                                                                padx=(0, 10))
        self.lang_box.grid(row=8, column=1, sticky="w", pady=(8, 4))

        self.check_label = self.text(self.body, "", pady=(14, 0))
        row_b = ttk.Frame(self.body)
        row_b.pack(side="bottom", fill="x")
        ttk.Button(row_b, text=t("‹  Atrás", "‹  Back"), command=self.page_connect).pack(side="left")
        self.send_button = ttk.Button(row_b, text=t("Comprobar y enviar  ›", "Check and send  ›"), style="Big.TButton",
                                      command=self.check_and_send)
        self.send_button.pack(side="right")

    def show_key_help(self):
        win = tk.Toplevel(self)
        win.title(t("Cómo conseguir tu clave de API", "How to get your API key"))
        win.geometry("560x380")
        f = ttk.Frame(win, padding=18)
        f.pack(fill="both", expand=True)
        steps = t("Cada contador usa su propia clave de la YouTube Data API v3. Es gratis y se hace una vez:\n\n"
                  "1. Pulsa el botón de abajo e inicia sesión con tu cuenta de Google.\n"
                  "2. Si te lo pide, crea un proyecto (cualquier nombre, por ejemplo «Contador»).\n"
                  "3. Pulsa «Habilitar» en «YouTube Data API v3».\n"
                  "4. Ve a «Credenciales» → «Crear credenciales» → «Clave de API».\n"
                  "5. Recomendado: en la clave, «Restringir clave» → solo «YouTube Data API v3».\n"
                  "6. Copia la clave (empieza por AIza) y pégala en el instalador.",
                  "Each counter uses its own YouTube Data API v3 key. It is free and you only do it once:\n\n"
                  "1. Click the button below and sign in with your Google account.\n"
                  "2. If asked, create a project (any name, for example \"Counter\").\n"
                  "3. Click \"Enable\" on \"YouTube Data API v3\".\n"
                  "4. Go to \"Credentials\" → \"Create credentials\" → \"API key\".\n"
                  "5. Recommended: on the key, \"Restrict key\" → only \"YouTube Data API v3\".\n"
                  "6. Copy the key (it starts with AIza) and paste it into the installer.")
        ttk.Label(f, text=steps, wraplength=510, justify="left").pack(anchor="w")
        ttk.Button(f, text=t("Abrir Google Cloud", "Open Google Cloud"), style="Big.TButton",
                   command=lambda: webbrowser.open(KEY_HELP)).pack(side="bottom", anchor="e")

    def check_and_send(self):
        ssid = self.ssid.get().replace(self.five_note, "").strip()
        key = self.key.get().strip()
        keep_key = not key and self.info and self.info.get("clave")
        if not ssid:
            return self.check_label.config(text=t("Elige o escribe tu red WiFi.", "Pick or type your WiFi network."),
                                           style="Err.TLabel")
        self.send_button.config(state="disabled")
        self.check_label.config(text=t("Comprobando el canal y la clave con YouTube…",
                                       "Checking the channel and the key with YouTube…"), style="TLabel")
        self.counter_lang = next((c for c, n in LANGUAGES.items() if n == self.lang_box.get()), LANG)
        config = {"wifi": ssid, "contrasena": self.password.get(), "canal": self.channel.get().strip(),
                  "idioma": self.counter_lang}
        if not keep_key:
            config["clave"] = key

        def work():
            if keep_key:
                ok = bool(config["canal"])
                msg = (t("Se mantiene la clave que ya tenía.", "The key it already had is kept.") if ok
                       else t("Escribe tu canal.", "Type your channel."))
            else:
                ok, msg = check_channel(config["canal"], key)
            self.post("checked", ok, msg, config)

        self.worker(work)

    def on_checked(self, ok, msg, config):
        if not self.send_button.winfo_exists():
            return
        if not ok:
            self.send_button.config(state="normal")
            return self.check_label.config(text="✗ " + msg, style="Err.TLabel")
        self.check_label.config(text="✓ " + msg, style="Ok.TLabel")
        self.page_sending(config, msg)

    # 5. Enviar y comprobar
    def page_sending(self, config, channel_msg):
        self.clear(t("Último paso", "Last step"), t("Enviando al contador", "Sending to the counter"))
        self.text(self.body, t(f"Canal: {channel_msg}", f"Channel: {channel_msg}"), style="Ok.TLabel")
        self.steps = {}
        for k, label in (("send", t("Enviar la configuración por el cable", "Send the settings over the cable")),
                         ("wifi", t(f"Conectarse a «{config['wifi']}»", f"Connect to \"{config['wifi']}\"")),
                         ("yt", t("Consultar YouTube", "Check YouTube"))):
            self.steps[k] = self.text(self.body, "○  " + label, pady=(10, 0))
        self.send_detail = self.text(self.body, "", style="Muted.TLabel", pady=12)
        self.worker(self.send_config, config)

    def send_config(self, config):
        try:
            s = open_and_reset(self.port)
        except serial.SerialException as e:
            return self.post("sent", "send", False, t(f"No se puede abrir {self.port}: {e}", f"Cannot open {self.port}: {e}"))
        with contextlib.closing(s):
            if not read_until(s, r"CONTADOR-YT LISTO", 6):
                return self.post("sent", "send", False, t("El contador no responde. Desconecta y vuelve a conectar el cable.",
                                                          "The counter does not answer. Unplug the cable and plug it in again."))
            s.write(("CONFIG " + json.dumps(config, ensure_ascii=False) + "\n").encode("utf-8"))
            if not read_until(s, r"^CONFIG OK", 3):
                return self.post("sent", "send", False, t("El contador no aceptó la configuración.",
                                                          "The counter did not accept the settings."))
            self.post("sent", "send", True, "")
            m = read_until(s, r"WiFi: conectado, IP (\S+)|WiFi: no conecta", 75)
            if not m or not m.group(1):
                return self.post("sent", "wifi", False,
                                 t("No consigue conectarse. Revisa la contraseña y que la red sea de 2,4 GHz.",
                                   "It cannot connect. Check the password and that the network is 2.4 GHz."))
            ip = m.group(1)
            self.post("sent", "wifi", True, ip)
            # Nombre fijo en la red (la IP la reparte el router y puede cambiar)
            m = read_until(s, r"Ajustes: http://(\S+\.local)", 15)
            host = m.group(1) if m else ""
            m = read_until(s, r"YouTube: (\d+)", 40)
            code = int(m.group(1)) if m else -1
            errors = {1: t("No llega a YouTube (¿la red tiene Internet?).", "It cannot reach YouTube (does the network have Internet?)."),
                      2: t("La clave no es válida para el contador.", "The key is not valid for the counter."),
                      3: t("La clave ha agotado su cuota de hoy.", "The key has used up today's quota."),
                      4: t("No encuentra el canal.", "It cannot find the channel.")}
            self.post("sent", "yt", code == 0,
                      (host, ip) if code == 0 else errors.get(code, t("No responde a tiempo.", "No answer in time.")))

    def on_sent(self, step, ok, detail):
        if not self.steps[step].winfo_exists():
            return
        text = self.steps[step].cget("text")[3:]
        self.steps[step].config(text=("✓  " if ok else "✗  ") + text, style="Ok.TLabel" if ok else "Err.TLabel")
        if not ok:
            self.send_detail.config(text=detail, style="Err.TLabel")
            row = ttk.Frame(self.body)
            row.pack(side="bottom", fill="x")
            ttk.Button(row, text=t("‹  Cambiar la configuración", "‹  Change the settings"), command=self.page_config).pack(side="left")
        elif step == "yt":
            self.page_done(*detail)

    def page_done(self, host, ip):
        self.clear(t("¡Listo!", "Done!"), t("Tu contador ya está funcionando", "Your counter is up and running"))
        self.text(self.body, t("Ya muestra los datos de tu canal. Puedes desconectarlo del ordenador y enchufarlo a "
                               "cualquier cargador de móvil.",
                               "It is already showing your channel stats. You can unplug it from the computer and plug "
                               "it into any phone charger."))
        self.text(self.body, t("Para cambiar el brillo, los avisos y más, abre su página de ajustes desde un móvil u "
                               "ordenador conectado a la misma WiFi:",
                               "To change the brightness, the alerts and more, open its settings page from a phone or "
                               "computer on the same WiFi:"), pady=(14, 4))
        url = f"http://{host or ip}"
        link = ttk.Label(self.body, text=url, foreground="#1a5fb4", cursor="hand2", font=("Segoe UI", 13))
        link.pack(anchor="w")
        link.bind("<Button-1>", lambda e: webbrowser.open(url))
        if host:
            self.text(self.body, t(f"Ese nombre no cambia. Si en algún móvil no abre, usa la dirección numérica "
                                   f"(ahora http://{ip}); esa la reparte el router y puede cambiar, pero el contador "
                                   "siempre enseña la actual en su pantalla «Ajustes» (tócala hasta que aparezca).",
                                   f"That name does not change. If it does not open on some phone, use the numeric "
                                   f"address (now http://{ip}); the router hands it out and it may change, but the counter "
                                   "always shows the current one on its \"Settings\" screen (tap until it appears)."),
                      style="Muted.TLabel", pady=(6, 0))
        self.shortcut_label = self.text(self.body, "", style="Ok.TLabel", pady=(10, 0))
        row = ttk.Frame(self.body)
        row.pack(side="bottom", fill="x")
        ttk.Button(row, text=t("Crear acceso directo en el escritorio", "Create a desktop shortcut"),
                   command=lambda: self.make_shortcut(url)).pack(side="left")
        ttk.Button(row, text=t("Terminar", "Finish"), style="Big.TButton", command=self.destroy).pack(side="right")

    def make_shortcut(self, url):
        """Un acceso directo (.url) en el escritorio que abre los ajustes del contador."""
        try:
            out = subprocess.run(["powershell", "-NoProfile", "-Command", "[Environment]::GetFolderPath('Desktop')"],
                                 capture_output=True, text=True, creationflags=NO_WINDOW).stdout.strip()
            desktop = Path(out) if out else Path.home() / "Desktop"
            name = t("Ajustes del contador YouTube", "YouTube Counter settings")
            shortcut = desktop / f"{name}.url"
            shortcut.write_text(f"[InternetShortcut]\nURL={url}\n", encoding="utf-8")
            self.shortcut_label.config(text=t(f"✓ Acceso directo «{name}» creado en el escritorio.",
                                              f"✓ Shortcut \"{name}\" created on the desktop."))
        except OSError as e:
            self.shortcut_label.config(text=t(f"No se pudo crear: {e}", f"Could not create it: {e}"), style="Err.TLabel")

if __name__ == "__main__":
    Installer().mainloop()
