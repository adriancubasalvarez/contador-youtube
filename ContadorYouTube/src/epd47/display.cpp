// Solo para la versión de tinta electrónica (entorno t5-epd47-s3 de PlatformIO).
#ifdef LILYGO_T5_EPD47_S3

// Dibujo en el framebuffer de 4 bits (16 grises) de la pantalla de 960x540.
#include "display.h"
#include <qrcode.h>
#include "epd_driver.h"
#include "fonts/fontbody.h"
#include "fonts/fonthero.h"
#include "fonts/fontlabel.h"
#include "fonts/fonttitle.h"
#include "fonts/fontvalue.h"
#include "../format.h"
#include "../history.h"
#include "utilities.h"

// Colores para formas (0x00 negro ... 0xFF blanco) y texto (0 ... 15)
#define C_BLACK 0x00
#define C_DARK 0x44
#define C_MID 0x88
#define C_LIGHT 0xCC
#define C_WHITE 0xFF
#define T_BLACK 0
#define T_DARK 5
#define T_MID 8
#define T_LIGHT 12
#define T_WHITE 15

#define MARGIN 32
#define HEADER_H 72
#define COL_SPLIT 530
#define RIGHT_X 564
#define ROW_SPLIT 338

static uint8_t *fb = nullptr;

enum Align { LEFT, RIGHT };

bool displayInit()
{
    fb = (uint8_t *)ps_calloc(1, EPD_WIDTH * EPD_HEIGHT / 2);
    if (!fb) return false;
    memset(fb, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    epd_init();
    return true;
}

void displayFlush()
{
    epd_poweron();
    epd_clear();
    epd_draw_grayscale_image(epd_full_screen(), fb);
    epd_poweroff_all();
}

float batteryVoltage()
{
    epd_poweron();  // el divisor de la batería se alimenta desde POWER_EN
    delay(10);
    float v = analogReadMilliVolts(BATT_PIN) * 2 / 1000.0f;
    epd_poweroff();
    return v;
}

static int batteryPercent(float v)
{
    int p = (int)((v - 3.3f) / (4.15f - 3.3f) * 100);
    return p < 0 ? 0 : p > 100 ? 100 : p;
}

// ------------------------------------------------------------------ texto ---

static int textWidth(const GFXfont *font, const String &s)
{
    if (!s.length()) return 0;
    int32_t x = 0, y = 0, x1, y1, w, h;
    get_text_bounds(font, s.c_str(), &x, &y, &x1, &y1, &w, &h, nullptr);
    return w;
}

static void text(const GFXfont *font, const String &s, int32_t x, int32_t y, uint8_t fg = T_BLACK,
                 uint8_t bg = T_WHITE, Align align = LEFT)
{
    if (!s.length()) return;
    if (align == RIGHT) x -= textWidth(font, s);
    FontProperties props = {};
    props.fg_color = fg;
    props.bg_color = bg;
    props.fallback_glyph = 0;  // los caracteres que no están en la fuente (emojis) se omiten
    write_mode(font, s.c_str(), &x, &y, fb, BLACK_ON_WHITE, &props);
}

static void dropLastChar(String &s)  // respeta UTF-8
{
    int i = s.length() - 1;
    while (i > 0 && ((uint8_t)s[i] & 0xC0) == 0x80) i--;
    s.remove(i < 0 ? 0 : i);
}

static String ellipsize(const GFXfont *font, String s, int maxW)
{
    if (textWidth(font, s) <= maxW) return s;
    while (s.length() && textWidth(font, s + "…") > maxW) dropLastChar(s);
    s.trim();
    return s + "…";
}

// Ajusta 's' en 'maxLines' líneas de ancho 'maxW'; la última se recorta con "…"
static int wrap(const GFXfont *font, const String &s, int maxW, int maxLines, String *lines)
{
    int n = 0, pos = 0;
    String cur;
    while (pos <= (int)s.length() && n < maxLines) {
        int sp = s.indexOf(' ', pos);
        if (sp < 0) sp = s.length();
        String word = s.substring(pos, sp);
        pos = sp + 1;
        if (!word.length()) continue;
        String cand = cur.length() ? cur + " " + word : word;
        if (textWidth(font, cand) <= maxW || !cur.length()) {
            cur = cand;
        } else {
            if (n == maxLines - 1) {
                lines[n++] = ellipsize(font, cand, maxW);
                return n;
            }
            lines[n++] = cur;
            cur = word;
        }
    }
    if (cur.length() && n < maxLines) lines[n++] = ellipsize(font, cur, maxW);
    return n;
}

// ---------------------------------------------------------------- bloques ---

static void header(const String &title, const String &right)
{
    epd_fill_rect(0, 0, EPD_WIDTH, HEADER_H, C_BLACK, fb);
    int rightW = textWidth(&FontLabel, right);
    text(&FontTitle, ellipsize(&FontTitle, title, EPD_WIDTH - 3 * MARGIN - rightW), MARGIN, 48, T_WHITE, T_BLACK);
    text(&FontLabel, right, EPD_WIDTH - MARGIN, 46, T_LIGHT, T_BLACK, RIGHT);
}

static void statBlock(const char *label, const String &value, int x, int y, int maxW)
{
    text(&FontLabel, label, x, y, T_DARK);
    text(&FontValue, ellipsize(&FontValue, value, maxW), x, y + 62);
}

static void chart(int32_t today, int x, int y, int w, int h)
{
    const int N = 14;
    int64_t days[N];
    historyDailyViews(today, N, days);
    int64_t peak = 0;
    int valid = 0;
    for (int i = 0; i < N; i++)
        if (days[i] >= 0) {
            valid++;
            if (days[i] > peak) peak = days[i];
        }

    text(&FontLabel, "VISTAS POR DÍA · 14 DÍAS", x, y, T_DARK);
    int top = y + 24, bottom = y + h;
    if (valid < 2) {
        text(&FontBody, "El gráfico aparecerá cuando haya", x, top + 52, T_MID);
        text(&FontBody, "al menos 2 días de datos.", x, top + 88, T_MID);
        return;
    }
    text(&FontLabel, "máx " + fmtCompact(peak), x + w, y, T_MID, T_WHITE, RIGHT);
    if (peak == 0) peak = 1;
    float slot = (float)w / N;
    int barW = (int)(slot * 0.7f);
    for (int i = 0; i < N; i++) {
        int bx = x + (int)(i * slot);
        if (days[i] < 0) {
            epd_fill_rect(bx, bottom - 2, barW, 2, C_LIGHT, fb);  // sin datos
            continue;
        }
        int bh = (int)((bottom - top) * days[i] / peak);
        if (bh < 2) bh = 2;
        // hoy, todavía parcial, en gris claro
        epd_fill_rect(bx, bottom - bh, barW, bh, i == N - 1 ? C_LIGHT : C_DARK, fb);
    }
    epd_draw_hline(x, bottom, w, C_MID, fb);
}

static void latestVideo(const ChannelStats &st, time_t now, int x, int y, int w)
{
    String label = "ÚLTIMO VÍDEO";
    String ago = fmtAgo(st.videoPublished, now);
    text(&FontLabel, label, x, y, T_DARK);
    if (ago.length()) text(&FontLabel, ago, x + w, y, T_MID, T_WHITE, RIGHT);
    if (!st.hasVideo) {
        text(&FontBody, "Sin vídeos públicos", x, y + 46, T_MID);
        return;
    }
    String lines[2];
    int n = wrap(&FontBody, st.videoTitle, w, 2, lines);
    for (int i = 0; i < n; i++) text(&FontBody, lines[i], x, y + 42 + i * 36);

    // "15,3 K  vistas · 1,2 K me gusta"
    String views = fmtCompact(st.videoViews);
    int valueW = textWidth(&FontValue, views) + 14;
    String rest = "vistas";
    if (st.videoLikes >= 0) {
        String withLikes = rest + " · " + fmtCompact(st.videoLikes) + " me gusta";
        if (textWidth(&FontBody, withLikes) <= w - valueW) rest = withLikes;
    }
    text(&FontValue, views, x, y + 134);
    text(&FontBody, rest, x + valueW, y + 134, T_DARK);
}

// -------------------------------------------------------------- pantallas ---

void drawDashboard(const ChannelStats &st, int32_t today, time_t now, bool stale)
{
    memset(fb, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    struct tm tm;
    localtime_r(&now, &tm);
    char clock[8];
    strftime(clock, sizeof(clock), "%H:%M", &tm);
    String right = String(stale ? "! sin conexión · " : "") + clock + " · " +
                   String(batteryPercent(batteryVoltage())) + " %";
    header(st.title, right);

    // Suscriptores
    int leftW = COL_SPLIT - 2 * MARGIN;
    text(&FontLabel, "SUSCRIPTORES", MARGIN, 120, T_DARK);
    String subs = st.subscribersHidden ? String("—") : fmtInt(st.subscribers);
    if (textWidth(&FontHero, subs) > leftW) subs = fmtCompact(st.subscribers);
    text(&FontHero, subs, MARGIN - 4, 262);

    int64_t delta = 0;
    int span = historySubscriberDelta(today, 7, st.subscribers, delta);
    if (span > 0 && !st.subscribersHidden) {
        String arrow = delta > 0 ? "↑ " : delta < 0 ? "↓ " : "";
        String amount = delta == 0 ? String("sin cambios") : arrow + fmtInt(delta < 0 ? -delta : delta);
        String when = span == 1 ? " desde ayer" : " en " + String(span) + " días";
        text(&FontTitle, amount, MARGIN, 312);
        text(&FontBody, when, MARGIN + textWidth(&FontTitle, amount) + 6, 312, T_DARK);
    } else {
        text(&FontBody, "Midiendo el crecimiento…", MARGIN, 312, T_MID);
    }

    // Totales del canal
    int rightW = EPD_WIDTH - MARGIN - RIGHT_X;
    statBlock("VISTAS TOTALES", fmtCompact(st.views), RIGHT_X, 120, rightW);
    int64_t todayViews[1];
    historyDailyViews(today, 1, todayViews);
    int half = rightW / 2;
    statBlock("VISTAS HOY", todayViews[0] >= 0 ? fmtCompact(todayViews[0]) : String("—"), RIGHT_X, 226, half - 12);
    statBlock("VÍDEOS", fmtInt(st.videos), RIGHT_X + half, 226, half);

    epd_draw_vline(COL_SPLIT, HEADER_H + 26, ROW_SPLIT - HEADER_H - 40, C_LIGHT, fb);
    epd_draw_hline(MARGIN, ROW_SPLIT, EPD_WIDTH - 2 * MARGIN, C_LIGHT, fb);

    chart(today, MARGIN, ROW_SPLIT + 40, leftW, 138);
    latestVideo(st, now, RIGHT_X, ROW_SPLIT + 40, rightW);
}

void drawSetupScreen(const char *apName)
{
    memset(fb, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    header("Dashboard YouTube", "Configuración");

    // Código QR para unirse a la WiFi del dispositivo (red abierta)
    String payload = String("WIFI:T:nopass;S:") + apName + ";;";
    QRCode qr;
    uint8_t data[qrcode_getBufferSize(3)];
    qrcode_initText(&qr, data, 3, ECC_LOW, payload.c_str());
    const int scale = 10, qx = MARGIN + 8, qy = HEADER_H + 60;
    epd_fill_rect(qx - 20, qy - 20, qr.size * scale + 40, qr.size * scale + 40, C_WHITE, fb);
    for (uint8_t y = 0; y < qr.size; y++)
        for (uint8_t x = 0; x < qr.size; x++)
            if (qrcode_getModule(&qr, x, y)) epd_fill_rect(qx + x * scale, qy + y * scale, scale, scale, C_BLACK, fb);

    int tx = qx + qr.size * scale + 56, ty = HEADER_H + 80;
    text(&FontValue, "Configúralo en 1 minuto", tx, ty + 30);
    const char *steps[][2] = {
        {"1. Escanea el código con la cámara", "del móvil para unirte a su WiFi."},
        {"2. Se abrirá una página: elige tu", "WiFi y escribe tu canal (@micanal)."},
        {"3. Listo. La pantalla se", "actualizará sola."},
    };
    for (int i = 0; i < 3; i++) {
        int y = ty + 104 + i * 92;
        text(&FontBody, steps[i][0], tx, y);
        text(&FontBody, steps[i][1], tx, y + 36, T_DARK);
    }
    text(&FontLabel, String("¿No se abre? Conéctate a la WiFi «") + apName + "» y entra en http://192.168.4.1",
         MARGIN, EPD_HEIGHT - 28, T_MID);
}

void drawMessage(const String &title, const String &line1, const String &line2)
{
    memset(fb, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    header("Dashboard YouTube", "");
    int w = EPD_WIDTH - 2 * MARGIN;
    text(&FontValue, ellipsize(&FontValue, title, w), MARGIN, 190);
    String lines[3];
    int n = wrap(&FontBody, line1, w, 2, lines);
    for (int i = 0; i < n; i++) text(&FontBody, lines[i], MARGIN, 260 + i * 40, T_DARK);
    if (line2.length()) text(&FontBody, ellipsize(&FontBody, line2, w), MARGIN, 260 + n * 40 + 24, T_DARK);
    text(&FontLabel, "Para cambiar la configuración: mantén pulsado el botón IO21 durante 3 segundos.", MARGIN,
         EPD_HEIGHT - 28, T_MID);
}

#endif
