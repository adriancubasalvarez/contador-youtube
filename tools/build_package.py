"""Monta la carpeta y el zip para usuarios (lo que se publica en el Release):

  ../Contador-YouTube-v<versión>/   y   ../Contador-YouTube-v<versión>.zip

con el instalador, los manuales (español e inglés), LEEME.txt y README.txt,
los firmwares, el ejemplo de config.txt, el código, las licencias y las
huellas SHA-256. Comprueba que nada lleva la clave de secrets.h.

Antes: tools/build_firmware.py, tools/instalador/build_exe.py y los manuales.

    python tools/build_package.py
"""
import hashlib
import re
import shutil
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
VERSION = re.search(r'#define FIRMWARE_VERSION "([^"]+)"',
                    (ROOT / "ContadorYouTube" / "src" / "version.h").read_text("utf-8")).group(1)
OUT = ROOT.parent / f"Contador-YouTube-v{VERSION}"


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def copy(src, dst):
    dst.parent.mkdir(parents=True, exist_ok=True)
    (shutil.copytree if src.is_dir() else shutil.copy2)(src, dst)


def text(name, body):
    (OUT / name).write_text(body.strip() + "\n", encoding="utf-8", newline="\r\n")  # Bloc de notas


if OUT.exists():
    shutil.rmtree(OUT)
exe = DIST / "InstaladorContadorYouTube.exe"
copy(exe, OUT / exe.name)
copy(ROOT / "docs" / "manual-contador-youtube.pdf", OUT / "Manual-Contador-YouTube.pdf")
copy(ROOT / "docs" / "manual-youtube-counter.pdf", OUT / "Manual-YouTube-Counter-EN.pdf")
for board in ("cyd", "oled"):
    copy(DIST / f"contador-{board}-v{VERSION}.bin", OUT / "firmware-para-actualizar" / f"contador-{board}-v{VERSION}.bin")
    copy(DIST / f"contador-{board}-v{VERSION}-completo.bin",
         OUT / "avanzado" / "firmware-completo" / f"contador-{board}-v{VERSION}-completo.bin")
copy(ROOT / "tarjeta-sd" / "config.txt.ejemplo", OUT / "tarjeta-sd" / "config.txt.ejemplo")
copy(ROOT / "ContadorYouTube", OUT / "avanzado" / "codigo-arduino" / "ContadorYouTube")
(OUT / "avanzado" / "codigo-arduino" / "ContadorYouTube" / "secrets.h").unlink(missing_ok=True)
copy(ROOT / "LICENSE.md", OUT / "avanzado" / "codigo-arduino" / "LICENSE.md")
inst = OUT / "avanzado" / "codigo-instalador"
for f in ("instalador.py", "build_exe.py", "LICENSE"):
    copy(ROOT / "tools" / "instalador" / f, inst / f)
copy(ROOT / "tools" / "instalador" / "img", inst / "img")
for f in ("LICENSE.md", "LICENCIA-MANUAL.txt", "AVISOS-TERCEROS.txt"):
    copy(ROOT / f, OUT / "licencias" / f)
copy(ROOT / "tools" / "instalador" / "LICENSE", OUT / "licencias" / "LICENCIA-INSTALADOR-GPL.txt")

H = sha(OUT / exe.name)
files = [exe.name] + [f"firmware-para-actualizar\\contador-{b}-v{VERSION}.bin" for b in ("cyd", "oled")] + \
        [f"avanzado\\firmware-completo\\contador-{b}-v{VERSION}-completo.bin" for b in ("cyd", "oled")]
sums = "\n".join(f"{sha(OUT / f.replace(chr(92), '/'))}  {f}" for f in files)

text("HUELLAS-SHA256.txt", f"""
Contador YouTube v{VERSION} · huellas SHA-256 · SHA-256 fingerprints (Adrian Cubas)

Si la huella que calcula tu PC coincide con la de esta lista, el archivo es el
original. Compárala también con la de la página de descarga (GitHub).
If the fingerprint your PC calculates matches this list, the file is the
original. Compare it with the one on the download page (GitHub) too.

PowerShell, en esta carpeta · in this folder:
    Get-FileHash .\\InstaladorContadorYouTube.exe

{sums}
""")

text("LEEME.txt", f"""
CONTADOR YOUTUBE · versión {VERSION}                    (English: README.txt)
================================

Un contador de escritorio con los suscriptores, las vistas y los vídeos de tu
canal de YouTube, en tiempo casi real. En español o en inglés.

Funciona con dos placas:
  - Pantalla táctil de 2,8" con ESP32 integrado ("Cheap Yellow Display",
    Freenove FNK0114F). Es el modelo recomendado.
  - ESP32 + pantallita OLED de 0,96" (SSD1306).


LO QUE NECESITAS
----------------
  1. Una de las dos placas y un cable USB de datos (no solo de carga).
  2. Un PC con Windows.
  3. Tu propia clave de la API de YouTube (gratis). El paso 1 del manual
     explica cómo conseguirla en unos 5 minutos. Cada contador usa la clave
     de su dueño: el programa no lleva ninguna.


EMPIEZA AQUÍ (3 pasos)
----------------------
  1. Abre Manual-Contador-YouTube.pdf y sigue el paso 1 para sacar tu clave.
  2. Conecta la placa al PC y abre InstaladorContadorYouTube.exe.
     Elige tu placa, tu WiFi, tu canal, pega la clave y elige el idioma del
     contador. El instalador carga el programa y lo deja configurado.
     (Windows puede avisar con "Windows protegió su PC": pulsa
      "Más información" y luego "Ejecutar de todas formas".)
  3. Listo. Para cambiar ajustes (también el idioma), abre en el navegador la
     dirección contador-xxxx.local que te muestra el instalador.


QUÉ HAY EN ESTA CARPETA
-----------------------
  InstaladorContadorYouTube.exe   Instala y configura el contador por USB.
  Manual-Contador-YouTube.pdf     Manual completo, paso a paso.
  Manual-YouTube-Counter-EN.pdf   El mismo manual, en inglés.

  firmware-para-actualizar\\       Para actualizar un contador ya instalado,
                                  sin cable:
                                  - desde la página de ajustes, enlace
                                    "Actualizar firmware" (al pie), eligiendo
                                    el .bin de tu placa;
                                  - o, en la pantalla táctil, copiando el .bin
                                    a la raíz de la microSD con el nombre
                                    firmware.bin y encendiendo el contador.
                                  (cyd = pantalla táctil, oled = pantalla OLED)

  tarjeta-sd\\config.txt.ejemplo   Opcional (pantalla táctil): configurar WiFi,
                                  canal, clave e idioma con la microSD, sin
                                  móvil ni PC. La tarjeta debe estar en FAT32.
                                  Sonidos propios: carpeta "sonidos" en la
                                  tarjeta con hito.wav, suscriptor.wav,
                                  comentario.wav o video.wav.

  avanzado\\                       Solo si no usas el instalador:
    firmware-completo\\            Imagen completa para grabar con otras
                                  herramientas (esptool, dirección 0x0).
    codigo-arduino\\               El código fuente, para compilarlo tú con
                                  Arduino IDE (paso 2B del manual).
    codigo-instalador\\            El código fuente del instalador.

  licencias\\                      Textos completos de las licencias.
  HUELLAS-SHA256.txt              Para comprobar que el instalador y los
                                  firmwares son los originales.


¿ES SEGURO EL INSTALADOR?
-------------------------
  El instalador no está firmado con un certificado de pago; por eso Windows
  avisa. Puedes comprobar que es el original con su huella SHA-256: abre
  PowerShell en esta carpeta y escribe

      Get-FileHash .\\InstaladorContadorYouTube.exe

  El resultado debe ser (sin importar mayúsculas):

      {H}

  La misma huella está publicada en la página de descarga. Su código fuente
  está en avanzado\\codigo-instalador.


LICENCIA
--------
  Copyright (c) 2026 Adrian Cubas.

  - Código y firmware: PolyForm Noncommercial 1.0.0. Puedes usarlo,
    modificarlo y compartirlo sin fines comerciales. Para venderlo
    (unidades montadas, kits o el firmware) pide permiso al autor.
  - Manuales: Creative Commons BY-NC 4.0 (citando al autor, sin fines
    comerciales).
  - Instalador de Windows: GPL-2.0-or-later, porque incluye esptool. Su código
    está en avanzado\\codigo-instalador.
  - Bibliotecas de terceros (LovyanGFX, WiFiManager, ArduinoJson, U8g2,
    Fira Sans, núcleo Arduino ESP32, esptool, Python...): sus licencias están
    en licencias\\AVISOS-TERCEROS.txt.

  Si compartes esta carpeta o el código, incluye la carpeta licencias.
""")

text("README.txt", f"""
YOUTUBE COUNTER · version {VERSION}                     (Español: LEEME.txt)
===================================

A desk counter showing your YouTube channel's subscribers, views and videos
in near real time. In English or Spanish.

It works with two boards:
  - 2.8" touch screen with a built-in ESP32 ("Cheap Yellow Display",
    Freenove FNK0114F). The recommended model.
  - ESP32 + a small 0.96" OLED screen (SSD1306).


WHAT YOU NEED
-------------
  1. One of the two boards and a USB data cable (not charge-only).
  2. A Windows PC.
  3. Your own YouTube API key (free). Step 1 of the manual explains how to
     get one in about 5 minutes. Every counter uses its owner's key: the
     program does not include one.


START HERE (3 steps)
--------------------
  1. Open Manual-YouTube-Counter-EN.pdf and follow step 1 to get your key.
  2. Connect the board to the PC and open InstaladorContadorYouTube.exe.
     It follows your Windows language; switch to English top right if
     needed. Pick your board, your WiFi, your channel, paste the key and
     choose the counter language. The installer writes the program and
     sets it up.
     (Windows may warn "Windows protected your PC": click "More info" and
      then "Run anyway".)
  3. Done. To change settings (the language too), open the address
     contador-xxxx.local shown by the installer in your browser.


WHAT IS IN THIS FOLDER
----------------------
  InstaladorContadorYouTube.exe   Installs and sets up the counter over USB.
  Manual-YouTube-Counter-EN.pdf   Full step-by-step manual.
  Manual-Contador-YouTube.pdf     The same manual, in Spanish.

  firmware-para-actualizar\\       To update a counter that is already
                                  installed, without a cable:
                                  - from the settings page, "Update firmware"
                                    link (at the bottom), picking the .bin
                                    for your board;
                                  - or, on the touch screen, copying the .bin
                                    to the root of the microSD as firmware.bin
                                    and powering on the counter.
                                  (cyd = touch screen, oled = OLED screen)

  tarjeta-sd\\config.txt.ejemplo   Optional (touch screen): set WiFi, channel,
                                  key and language with the microSD, without
                                  a phone or PC. The card must be FAT32.
                                  Your own sounds: a "sonidos" folder on the
                                  card with hito.wav (milestone),
                                  suscriptor.wav (subscriber), comentario.wav
                                  (comment) or video.wav.

  avanzado\\                       Only if you do not use the installer:
    firmware-completo\\            Full image to write with other tools
                                  (esptool, address 0x0).
    codigo-arduino\\               The source code, to build it yourself with
                                  Arduino IDE (step 2B of the manual).
    codigo-instalador\\            The installer's source code.

  licencias\\                      Full license texts.
  HUELLAS-SHA256.txt              To check that the installer and firmware
                                  files are the originals.


IS THE INSTALLER SAFE?
----------------------
  The installer is not signed with a paid certificate, which is why Windows
  warns. You can check it is the original with its SHA-256 fingerprint: open
  PowerShell in this folder and type

      Get-FileHash .\\InstaladorContadorYouTube.exe

  The result must be (upper or lower case does not matter):

      {H}

  The same fingerprint is published on the download page. Its source code is
  in avanzado\\codigo-instalador.


LICENSE
-------
  Copyright (c) 2026 Adrian Cubas.

  - Code and firmware: PolyForm Noncommercial 1.0.0. You may use, modify and
    share it for non-commercial purposes. To sell it (assembled units, kits
    or the firmware) ask the author for permission.
  - Manuals: Creative Commons BY-NC 4.0 (crediting the author,
    non-commercial).
  - Windows installer: GPL-2.0-or-later, because it bundles esptool. Its code
    is in avanzado\\codigo-instalador.
  - Third-party libraries (LovyanGFX, WiFiManager, ArduinoJson, U8g2,
    Fira Sans, Arduino ESP32 core, esptool, Python...): their licenses are in
    licencias\\AVISOS-TERCEROS.txt.

  If you share this folder or the code, include the licencias folder.
""")

# Nada con la clave de pruebas
secrets = ROOT / "ContadorYouTube" / "secrets.h"
keys = re.findall(r'"([^"]{20,})"', secrets.read_text("utf-8")) if secrets.exists() else []
leaks = [p for p in OUT.rglob("*") if p.is_file() and any(k.encode() in p.read_bytes() for k in keys)]
if leaks:
    sys.exit(f"¡La clave de secrets.h está en {leaks}! No se publica.")

zip_path = OUT.parent / f"{OUT.name}.zip"
zip_path.unlink(missing_ok=True)
with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
    for p in sorted(OUT.rglob("*")):
        if p.is_file():
            z.write(p, p.relative_to(OUT).as_posix())
print(f"{OUT}\n{zip_path}  ({zip_path.stat().st_size // (1024 * 1024)} MB)\ninstalador {H}")
