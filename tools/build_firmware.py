"""Genera los archivos del firmware para publicar, de las dos placas, en dist/:

  contador-oled-v1.2.0.bin            actualización (tarjeta SD o página de ajustes)
  contador-oled-v1.2.0-completo.bin   instalación desde cero por USB (en 0x0)
  contador-cyd-v1.2.0.bin
  contador-cyd-v1.2.0-completo.bin

Usa arduino-cli con el núcleo esp32 y las librerías ya instaladas (las mismas
que Arduino IDE):

  python tools/build_firmware.py
  ARDUINO_CLI=ruta/arduino-cli ARDUINO_CLI_CONFIG=ruta/arduino-cli.yaml python tools/build_firmware.py

El firmware publicado no lleva clave de API (se compila con PUBLIC_BUILD y se
comprueba): cada usuario pone la suya al configurarlo. La de secrets.h queda
solo para las pruebas propias.

Para el .bin de actualización, el usuario lo copia a la tarjeta como
firmware.bin o lo sube en http://contador-xxxx.local/actualizar.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKETCH = ROOT / "ContadorYouTube"
DIST = ROOT / "dist"
FQBN = "esp32:esp32:esp32:PartitionScheme=min_spiffs"
# PUBLIC_BUILD: sin la clave de API de secrets.h. Cada usuario pone la suya.
BOARDS = {"oled": "-DPUBLIC_BUILD", "cyd": "-DPUBLIC_BUILD -DBOARD_CYD"}


def main():
    version = re.search(r'#define FIRMWARE_VERSION "([^"]+)"', (SKETCH / "src" / "version.h").read_text("utf-8")).group(1)
    cli = [os.environ.get("ARDUINO_CLI", "arduino-cli")]
    if os.environ.get("ARDUINO_CLI_CONFIG"):
        cli += ["--config-file", os.environ["ARDUINO_CLI_CONFIG"]]

    secrets = SKETCH / "secrets.h"
    own_key = re.search(r'YT_API_KEY\s+"(AIza[^"]+)"', secrets.read_text("utf-8")) if secrets.exists() else None

    DIST.mkdir(exist_ok=True)
    for board, flags in BOARDS.items():
        with tempfile.TemporaryDirectory() as build:
            cmd = cli + ["compile", "--fqbn", FQBN, "--build-path", build, str(SKETCH)]
            if flags:
                cmd += ["--build-property", f"compiler.cpp.extra_flags={flags}"]
            print(f"Compilando {board} v{version}...")
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode:
                print(r.stdout[-3000:], r.stderr[-3000:])
                sys.exit(f"Error al compilar {board}")
            name = f"contador-{board}-v{version}"
            app = (Path(build) / "ContadorYouTube.ino.bin").read_bytes()
            if own_key and own_key.group(1).encode() in app:  # nunca se publica una clave
                sys.exit(f"Error: el firmware de {board} lleva la clave de secrets.h. No se publica.")
            shutil.copy(Path(build) / "ContadorYouTube.ino.bin", DIST / f"{name}.bin")
            shutil.copy(Path(build) / "ContadorYouTube.ino.merged.bin", DIST / f"{name}-completo.bin")
            print(f"  dist/{name}.bin  ({(DIST / f'{name}.bin').stat().st_size // 1024} KB)")
            print(f"  dist/{name}-completo.bin")


if __name__ == "__main__":
    main()
