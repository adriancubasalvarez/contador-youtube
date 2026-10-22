#!/bin/sh
# Compila y ejecuta la vista previa de la CYD en el PC (Windows, Linux o macOS)
# y monta docs/preview-cyd.png (vistas principales) y preview-cyd-detalles.png
# (las que se abren al tocar).
#
# Necesita:
#   - zig como compilador de C++ (pip install ziglang; o cualquier g++/clang++)
#   - LovyanGFX (la misma que usa el sketch) en $LGFX
#   - SDL2 de desarrollo en $SDL2 (LovyanGFX la usa como plataforma de PC;
#     en Windows: SDL2-devel-*-mingw de github.com/libsdl-org/SDL/releases,
#     carpeta x86_64-w64-mingw32)
#   - Python con Pillow para montar la imagen final
#
#   LGFX=~/Arduino/libraries/LovyanGFX SDL2=.../x86_64-w64-mingw32 OUT=/tmp/cyd sh build.sh
set -e
cd "$(dirname "$0")"  # rutas relativas: la carpeta del proyecto lleva espacios
HERE=.
ROOT=../..
OUT=${OUT:-"$HERE/out"}
CXX=${CXX:-"python -m ziglang c++"}
CC=${CC:-"python -m ziglang cc"}
mkdir -p "$OUT/obj"

FLAGS="-O1 -DLGFX_SDL -DCYD_PREVIEW -DBOARD_CYD -Ishim -I. -I$LGFX/src -I$SDL2/include -I$SDL2/include/SDL2"
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) FLAGS="$FLAGS -target x86_64-windows-gnu"; EXE=preview.exe ;; *) EXE=preview ;; esac

for f in "$LGFX"/src/lgfx/utility/*.c "$LGFX"/src/lgfx/Fonts/efont/*.c "$LGFX"/src/lgfx/Fonts/IPA/*.c \
         "$LGFX"/src/lgfx/Fonts/lvgl/*.c "$LGFX"/src/lgfx/v1/lv_font/*.c; do
    [ -f "$f" ] && $CC $FLAGS -c "$f" -o "$OUT/obj/$(basename "$f").o"
done
for f in "$LGFX"/src/lgfx/v1/*.cpp "$LGFX"/src/lgfx/v1/panel/*.cpp "$LGFX"/src/lgfx/v1/touch/*.cpp \
         "$ROOT/ContadorYouTube/src/cyd/screen.cpp" "$ROOT/ContadorYouTube/src/format.cpp" \
         "$ROOT/ContadorYouTube/src/trends.cpp" "$HERE/preview.cpp"; do
    $CXX -std=gnu++17 $FLAGS -c "$f" -o "$OUT/obj/$(basename "$f").o"
done
SDLLIB="$SDL2/lib/libSDL2.dll.a"; [ -f "$SDLLIB" ] || SDLLIB="-L$SDL2/lib -lSDL2"
$CXX $FLAGS "$OUT"/obj/*.o $SDLLIB -o "$OUT/$EXE"
[ -f "$SDL2/bin/SDL2.dll" ] && cp "$SDL2/bin/SDL2.dll" "$OUT/"
"$OUT/$EXE" "$OUT"
python "$HERE/sheet.py" "$OUT" "$ROOT/docs/preview-cyd.png"
python "$HERE/sheet.py" "$OUT" "$ROOT/docs/preview-cyd-detalles.png" "d*.ppm"
