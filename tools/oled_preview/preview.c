/* Vista previa en el PC de las pantallas del OLED (src/oled/screen.cpp),
 * dibujadas con la librería U8g2 real y las mismas fuentes y coordenadas.
 * Los datos son de ejemplo. Escribe un PGM por pantalla en la carpeta dada.
 *
 * Compilar (ver tools/oled_preview/build.sh):
 *   cc -I<U8g2>/src/clib preview.c <U8g2>/src/clib/*.c -o preview
 */
#include <stdio.h>
#include <string.h>
#include "u8g2.h"

#define W 128
#define H 64
static u8g2_t g;

/* Se usa el mismo controlador que en la placa (SSD1306 128x64 con búfer
 * completo), pero sin enviar nada: se lee el búfer directamente. */
static uint8_t dummy_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) { return 1; }

static int width(const char *s) { return u8g2_GetUTF8Width(&g, s); }
static void text(int x, int y, const char *s) { u8g2_DrawUTF8(&g, x, y, s); }
static void textRight(int x, int y, const char *s) { u8g2_DrawUTF8(&g, x - width(s), y, s); }

static void header(const char *left, const char *right)
{
    u8g2_SetFont(&g, u8g2_font_helvB08_tf);
    text(0, 11, left);
    u8g2_SetFont(&g, u8g2_font_helvR08_tf);
    if (right) textRight(W, 11, right);
}

static void bigNumber(const char *full, const char *compact)
{
    const uint8_t *fonts[] = {u8g2_font_logisoso32_tr, u8g2_font_logisoso28_tr, u8g2_font_logisoso24_tr,
                              u8g2_font_logisoso20_tr, u8g2_font_logisoso16_tr};
    const char *values[] = {full, compact};
    for (int v = 0; v < 2; v++)
        for (int f = 0; f < 5; f++) {
            u8g2_SetFont(&g, fonts[f]);
            int w = width(values[v]);
            if (w <= W - 4) {
                text((W - w) / 2, 40 + u8g2_GetAscent(&g) / 2, values[v]);
                return;
            }
        }
}

static void save(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.pgm", dir, name);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P5 %d %d 255\n", W, H);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) fputc((u8g2_GetBufferPtr(&g)[(y / 8) * W + x] >> (y % 8)) & 1 ? 255 : 0, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&g, U8G2_R0, dummy_cb, dummy_cb);
    u8g2_InitDisplay(&g);
    u8g2_SetPowerSave(&g, 0);
    u8g2_SetFontPosBaseline(&g);

    u8g2_ClearBuffer(&g);
    header("SUSCRIPTORES", "+1,2 K 7d");
    bigNumber("128.400", "128,4 K");
    u8g2_SendBuffer(&g);
    save(dir, "1_suscriptores");

    u8g2_ClearBuffer(&g);
    header("VISTAS", "hoy +3.412");
    bigNumber("9.876.543", "9,9 M");
    u8g2_SendBuffer(&g);
    save(dir, "2_vistas");

    u8g2_ClearBuffer(&g);
    header("ÚLTIMO VÍDEO", "3 d");
    u8g2_SetFont(&g, u8g2_font_helvR08_tf);
    text(1, 27, "Cómo construí un dashboard");
    text(1, 38, "E-Ink con una Raspberry...");
    u8g2_SetFont(&g, u8g2_font_helvB12_tf);
    text(1, 62, "15,3 K");
    int vw = width("15,3 K");
    u8g2_SetFont(&g, u8g2_font_helvR08_tf);
    text(1 + vw + 4, 62, "vistas");
    u8g2_SendBuffer(&g);
    save(dir, "3_video");

    u8g2_ClearBuffer(&g);
    header("VISTAS/DÍA", "máx 4,2 K");
    int vals[14] = {2100, 2600, 1900, 3300, 4186, 3900, 2500, 2200, 2800, 3500, 3100, 2700, 3600, 1400};
    const int top = 19, bottom = 63, slot = 9, barW = 7, x0 = (W - 14 * slot) / 2 + 1;
    for (int i = 0; i < 14; i++) {
        int h = (bottom - top) * vals[i] / 4186;
        if (i == 13)
            u8g2_DrawFrame(&g, x0 + i * slot, bottom - h + 1, barW, h);
        else
            u8g2_DrawBox(&g, x0 + i * slot, bottom - h + 1, barW, h);
    }
    u8g2_SendBuffer(&g);
    save(dir, "4_grafico");

    u8g2_ClearBuffer(&g);
    header("AJUSTES", NULL);
    u8g2_SetFont(&g, u8g2_font_helvR08_tf);
    text(0, 27, "Abre en el navegador:");
    u8g2_SetFont(&g, u8g2_font_helvB08_tf);
    text(0, 40, "contador-08f0.local");
    u8g2_SetFont(&g, u8g2_font_helvR08_tf);
    text(0, 52, "o bien:");
    u8g2_SetFont(&g, u8g2_font_helvB08_tf);
    text(0, 63, "192.168.1.147");
    u8g2_SendBuffer(&g);
    save(dir, "5_ajustes");

    u8g2_ClearBuffer(&g);
    header("CONFIGURACIÓN", NULL);
    u8g2_SetFont(&g, u8g2_font_helvR08_tf);
    text(0, 27, "1. Conecta el móvil a:");
    u8g2_SetFont(&g, u8g2_font_helvB10_tf);
    text(8, 41, "YT-Dashboard");
    u8g2_SetFont(&g, u8g2_font_helvR08_tf);
    text(0, 52, "2. Sigue los pasos");
    text(0, 63, "(o abre 192.168.4.1)");
    u8g2_SendBuffer(&g);
    save(dir, "0_configuracion");
    return 0;
}
