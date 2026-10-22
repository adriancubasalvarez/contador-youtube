"""Empaqueta el instalador en un solo .exe para Windows, con el firmware dentro:

  dist/InstaladorContadorYouTube.exe

Antes: python tools/build_firmware.py (deja en dist/ los -completo.bin).
Necesita: pip install pyinstaller esptool pyserial
"""
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
DIST = HERE.parents[1] / "dist"
NAME = "InstaladorContadorYouTube"


def main():
    def version(f):
        return tuple(int(n) for n in re.search(r"-v(\d+)\.(\d+)\.(\d+)-completo", f.name).groups())

    newest = {}  # placa -> el -completo.bin de versión más alta
    for f in DIST.glob("contador-*-v*-completo.bin"):
        board = f.name.split("-")[1]
        if board not in newest or version(f) > version(newest[board]):
            newest[board] = f
    firmware = sorted(newest.values())
    boards = set(newest)
    if boards != {"oled", "cyd"}:
        sys.exit("Faltan los firmware en dist/: ejecuta antes tools/build_firmware.py")
    work = HERE / "build"
    sep = ";" if sys.platform == "win32" else ":"
    cmd = [sys.executable, "-m", "PyInstaller", "--noconfirm", "--onefile", "--windowed", "--name", NAME,
           "--icon", str(HERE / "img" / "icono.ico"),
           "--add-data", f"{HERE / 'img'}{sep}img",
           "--collect-data", "esptool", "--collect-submodules", "esptool",
           # librerías del mismo entorno que el instalador no usa (fuentes, vista previa, cámara)
           "--exclude-module", "cv2", "--exclude-module", "numpy", "--exclude-module", "PIL",
           "--exclude-module", "freetype",
           "--distpath", str(DIST), "--workpath", str(work), "--specpath", str(work)]
    for f in firmware:  # solo la versión más reciente de cada placa viaja dentro
        cmd += ["--add-data", f"{f}{sep}firmware"]
    cmd.append(str(HERE / "instalador.py"))
    subprocess.run(cmd, check=True)
    shutil.rmtree(work, ignore_errors=True)
    exe = DIST / f"{NAME}.exe"
    print(f"\n{exe}  ({exe.stat().st_size // (1024 * 1024)} MB)")


if __name__ == "__main__":
    main()
