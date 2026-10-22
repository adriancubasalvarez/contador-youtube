// Solo para la pantalla táctil de 2,8" con ESP32 integrado ("Cheap Yellow
// Display"): se activa con #define BOARD_CYD en config.h.
#include "../../config.h"
#if defined(BOARD_CYD) && !defined(LILYGO_T5_EPD47_S3)

#include "../screen.h"
#include <LovyanGFX.hpp>
#include "../format.h"
#include "../history.h"
#include "../lang.h"
#include "../trends.h"
#include "fonts.h"
#ifndef CYD_PREVIEW
#include <Preferences.h>
#endif

// Pines de la Freenove FNK0114F (2,8" ILI9341), los mismos que la CYD
// estándar ESP32-2432S028. Pantalla por un bus SPI y táctil XPT2046 por otro.
#define TFT_SCLK 14
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_CS 15
#define TFT_DC 2
#define TFT_BL 21
#define TOUCH_SCLK 25
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CS 33

#ifdef CYD_PREVIEW
#include "cyd_preview_hw.h"  // tools/cyd_preview: la misma pantalla simulada en el PC
#else
class CydDisplay : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 panel;
    lgfx::Bus_SPI bus;
    lgfx::Light_PWM light;

public:
    CydDisplay()
    {
        auto b = bus.config();
        b.spi_host = SPI2_HOST;  // HSPI
        b.spi_mode = 0;
        b.freq_write = 40000000;
        b.freq_read = 16000000;
        b.pin_sclk = TFT_SCLK;
        b.pin_mosi = TFT_MOSI;
        b.pin_miso = TFT_MISO;
        b.pin_dc = TFT_DC;
        bus.config(b);
        panel.setBus(&bus);

        auto p = panel.config();
        p.pin_cs = TFT_CS;
        p.pin_rst = -1;  // va al reset de la placa
        p.panel_width = 240;
        p.panel_height = 320;
        p.readable = true;
        p.invert = false;
        p.rgb_order = false;
        p.bus_shared = false;
        panel.config(p);

        auto l = light.config();
        l.pin_bl = TFT_BL;
        l.freq = 12000;
        l.pwm_channel = 7;
        light.config(l);
        panel.setLight(&light);

        setPanel(&panel);  // el táctil va aparte (ver readTouch)
    }
};
#endif

#define W 320
#define H 240
// La imagen se compone por franjas en la RAM y luego se envía, para que no
// parpadee. Una imagen entera (150 KB) no cabe junto a la WiFi y el TLS.
#define BAND_H 48

static constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static constexpr uint16_t C_BG = rgb(12, 12, 14);
static constexpr uint16_t C_PANEL = rgb(30, 30, 35);
static constexpr uint16_t C_LINE = rgb(44, 44, 52);
static constexpr uint16_t C_TEXT = rgb(245, 245, 247);
static constexpr uint16_t C_MUTED = rgb(150, 150, 160);
static constexpr uint16_t C_RED = rgb(255, 0, 51);  // rojo de YouTube
static constexpr uint16_t C_RED_DIM = rgb(105, 18, 36);
static constexpr uint16_t C_RED_LINE = rgb(160, 30, 55);  // borde de lo que se puede tocar
static constexpr uint16_t C_HINT = rgb(125, 125, 135);
static constexpr uint16_t C_UP = rgb(70, 205, 120);
static constexpr uint16_t C_DOWN = rgb(255, 95, 95);
static constexpr uint16_t C_WARN = rgb(255, 180, 0);
static constexpr uint16_t C_WHITE = rgb(255, 255, 255);

static CydDisplay lcd;
static LGFX_Sprite band(&lcd);
static lgfx::LovyanGFX *cv = &lcd;  // donde se dibuja ahora: la franja o la pantalla
static int oy = 0;                  // fila de la pantalla donde empieza la franja
static const uint8_t *loadedFont = nullptr;

// Zonas táctiles de lo último que se dibujó: cada vista las registra al
// dibujarse, así coinciden siempre con lo que se ve
struct Zone {
    int16_t x, y, w, h;
    TouchTarget target;
    int8_t index;
};
static Zone zones[24];
static uint16_t mix(uint16_t a, uint16_t b, float t)  // de a (t=0) a b (t=1)
{
    auto ch = [&](int sh, int mask) {
        int ca = (a >> sh) & mask, cb = (b >> sh) & mask;
        return (uint16_t)((int)(ca + (cb - ca) * t) & mask) << sh;
    };
    return ch(11, 31) | ch(5, 63) | ch(0, 31);
}

// Lo que se puede tocar tiene siempre el mismo aspecto (recuadro con borde
// rojo y "›"). Mientras el usuario no haya usado una zona, cada pantalla
// lleva una pista de una línea y el borde "late" al aparecer; al usarla una
// vez, se recuerda (también tras apagar) y la pantalla queda tranquila.
enum Learned : uint8_t { L_SUBSCRIBERS = 1, L_DAY = 2, L_VIDEOS = 4, L_COMMENTS = 8 };
static uint8_t learned = 0;
static bool pulseOn = false;
static uint32_t pulseStart = 0;
#define PULSE_MS 1300

static void loadLearned()
{
#ifndef CYD_PREVIEW
    Preferences p;
    if (p.begin("ytui", true)) {
        learned = p.getUChar("learned", 0);
        p.end();
    }
#endif
}

static void learn(TouchTarget t)
{
    uint8_t bit = t == TOUCH_SUBSCRIBERS ? L_SUBSCRIBERS
                  : t == TOUCH_DAY       ? L_DAY
                  : t == TOUCH_VIDEOS    ? L_VIDEOS
                  : t == TOUCH_COMMENTS  ? L_COMMENTS
                                         : 0;
    if (!bit || (learned & bit)) return;
    learned |= bit;
#ifndef CYD_PREVIEW
    Preferences p;
    if (p.begin("ytui", false)) {
        p.putUChar("learned", learned);
        p.end();
    }
#endif
}

// 0-1: brillo extra del borde durante el latido (solo si aún no se ha usado)
static float pulse(uint8_t bit)
{
    if (!pulseOn || (learned & bit)) return 0;
    float t = (float)(millis() - pulseStart) / PULSE_MS;
    return t >= 1 ? 0 : sinf(PI * t);
}

// Borde de un elemento tocable: rojo apagado; durante el latido, más vivo y doble
static void touchBorder(int x, int y, int w, int h, int r, uint8_t bit)
{
    float k = pulse(bit);
    uint16_t c = k > 0 ? mix(C_RED_LINE, C_RED, k) : C_RED_LINE;
    cv->drawRoundRect(x, y - oy, w, h, r, c);
    if (k > 0.35f) cv->drawRoundRect(x - 1, y - 1 - oy, w + 2, h + 2, r + 1, c);
}

static bool bubbleActive = false;  // "+N" en lugar de la etiqueta de suscriptores
#define TOAST_ZONE 99  // índice de la zona del aviso deslizante (se cierra al tocarlo)
static void dismissToast();
static int zoneCount = 0;

static void zone(int x, int y, int w, int h, TouchTarget target, int index = 0)
{
    if (oy != 0 || zoneCount >= (int)(sizeof(zones) / sizeof(zones[0]))) return;  // solo en la primera franja
    zones[zoneCount++] = {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, target, (int8_t)index};
}

// Compone la pantalla franja a franja con draw(). Si no hubiera memoria para
// la franja, dibuja directamente en la pantalla.
static bool lastWasPage = false;  // lo último dibujado fue una vista principal (para los efectos)
static void stopConfetti();

template <typename F> static void present(F draw)
{
    zoneCount = 0;
    lastWasPage = false;
    stopConfetti();
    if (band.getBuffer()) {
        cv = &band;
        for (oy = 0; oy < H; oy += BAND_H) {
            band.fillScreen(C_BG);
            draw();
            band.pushSprite(0, oy);
        }
    } else {
        cv = &lcd;
        oy = 0;
        loadedFont = nullptr;
        lcd.startWrite();
        lcd.fillScreen(C_BG);
        draw();
        lcd.endWrite();
    }
}

static void font(const uint8_t *f)
{
    if (f == loadedFont) return;
    cv->loadFont(f);
    loadedFont = f;
}

static int width(const String &s) { return cv->textWidth(s.c_str()); }

// Texto con la línea base en y; align: 0 izquierda, 1 centro, 2 derecha
static void text(int x, int y, const String &s, uint16_t color, int align = 0)
{
    static const lgfx::textdatum_t datum[] = {lgfx::baseline_left, lgfx::baseline_center, lgfx::baseline_right};
    cv->setTextDatum(datum[align]);
    cv->setTextColor(color);
    cv->drawString(s.c_str(), x, y - oy);
}

static void box(int x, int y, int w, int h, uint16_t c) { cv->fillRect(x, y - oy, w, h, c); }
static void roundBox(int x, int y, int w, int h, int r, uint16_t c) { cv->fillSmoothRoundRect(x, y - oy, w, h, r, c); }

static String ellipsize(String s, int maxW)
{
    if (width(s) <= maxW) return s;
    while (s.length() && width(s + "…") > maxW) {
        int i = s.length() - 1;
        while (i > 0 && ((uint8_t)s[i] & 0xC0) == 0x80) i--;  // respeta UTF-8
        s.remove(i);
    }
    s.trim();
    return s + "…";
}

static int wrap(const String &s, int maxW, int maxLines, String *lines)
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
        if (width(cand) <= maxW || !cur.length()) {
            cur = cand;
        } else if (n == maxLines - 1) {
            lines[n++] = ellipsize(cand, maxW);
            return n;
        } else {
            lines[n++] = cur;
            cur = word;
        }
    }
    if (cur.length() && n < maxLines) lines[n++] = ellipsize(cur, maxW);
    return n;
}

static void logo(int x, int y)
{
    roundBox(x, y, 30, 21, 6, C_RED);
    cv->fillTriangle(x + 12, y + 6 - oy, x + 12, y + 15 - oy, x + 20, y + 10 - oy, C_WHITE);
}

// Cabecera: logo, nombre del canal y la hora (en ámbar si los datos no están al día)
static void header(const String &title, time_t now, bool stale, int battery = -1)
{
    logo(12, 9);
    String clock;
    if (now > 1700000000) {
        struct tm tm;
        localtime_r(&now, &tm);
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", tm.tm_hour, tm.tm_min);
        clock = buf;
    }
    font(FontLabel);
    int clockW = clock.length() ? width(clock) : 0;
    text(W - 12, 26, clock, stale ? C_WARN : C_MUTED, 2);
    int right = W - 12 - clockW;  // borde izquierdo de lo que hay a la derecha
    if (battery >= 0) {
        // Pila con el nivel dentro y el % al lado
        uint16_t c = battery <= 15 ? C_DOWN : C_MUTED;
        int bx = right - 10 - 24;
        cv->drawRoundRect(bx, 14 - oy, 22, 12, 2, c);
        box(bx + 22, 17, 2, 6, c);
        box(bx + 2, 16, (18 * battery + 50) / 100, 8, battery <= 15 ? C_DOWN : C_UP);
        font(FontSmall);
        String pct = String(battery) + "%";
        text(bx - 4, 25, pct, c, 2);
        right = bx - 4 - width(pct);
        font(FontLabel);
    }
    if (stale) {
        cv->fillSmoothCircle(right - 10, 21 - oy, 4, C_WARN);
        right -= 14;
    }
    text(50, 26, ellipsize(title, right - 50 - 12), C_TEXT);
    box(0, 40, W, 1, C_LINE);
}

static void dots(int active)
{
    const int gap = 14, x0 = W / 2 - (PAGE_COUNT - 1) * gap / 2;
    for (int i = 0; i < PAGE_COUNT; i++)
        cv->fillSmoothCircle(x0 + i * gap, 231 - oy, i == active ? 4 : 3, i == active ? C_RED : C_LINE);
}

// Cifra grande centrada: la mayor fuente en la que quepa, primero con todas
// las cifras y si no, abreviada
static void bigNumber(int cy, const String &full, const String &compact, int maxW = W - 24)
{
    for (const String &value : {full, compact}) {
        for (const uint8_t *f : {FontHero, FontBig, FontValue}) {
            font(f);
            if (width(value) <= maxW) {
                int h = f == FontHero ? 54 : f == FontBig ? 43 : 32;  // altura de las cifras
                text(W / 2, cy + h / 2, value, C_TEXT, 1);
                return;
            }
        }
    }
}

// Etiqueta en una píldora centrada, con un triángulo de tendencia opcional.
// Si 'target' no es TOUCH_NONE, la píldora se puede tocar (lleva "›").
static void pill(int y, const String &label, uint16_t color, int trend, TouchTarget target = TOUCH_NONE, int index = 0,
                 uint8_t bit = 0)
{
    bool touchable = target != TOUCH_NONE;
    font(FontBody);
    int tw = width(label), iw = trend ? 16 : 0, cw = touchable ? 18 : 0, w = tw + iw + cw + 28, x = (W - w) / 2;
    roundBox(x, y, w, 30, 15, C_PANEL);
    if (touchable) touchBorder(x, y, w, 30, 15, bit);
    int tx = x + 14;
    if (trend > 0) cv->fillTriangle(tx, y + 20 - oy, tx + 11, y + 20 - oy, tx + 5, y + 10 - oy, color);
    if (trend < 0) cv->fillTriangle(tx, y + 10 - oy, tx + 11, y + 10 - oy, tx + 5, y + 20 - oy, color);
    text(tx + iw, y + 21, label, color);
    if (touchable) {
        text(x + w - 14, y + 21, "›", C_RED, 2);
        zone(x - 10, y - 10, w + 20, 50, target, index);
    }
}

// Pista de una línea encima de los puntos de página
static void hint(const String &s)
{
    font(FontSmall);
    text(W / 2, 217, s, C_HINT, 1);
}

static void label(int y, const String &s)
{
    font(FontLabel);
    text(W / 2, y, s, C_MUTED, 1);
}

static void drawSubscribers(const ChannelStats &st, int32_t today)
{
    if (!bubbleActive) label(70, TR("SUSCRIPTORES", "SUBSCRIBERS"));
    if (st.subscribersHidden) {
        font(FontStrong);
        text(W / 2, 135, TR("Contador oculto", "Count hidden"), C_MUTED, 1);
        return;
    }
    bigNumber(124, fmtInt(st.subscribers), fmtCompact(st.subscribers));
    int64_t delta = 0;
    int span = historySubscriberDelta(today, 7, st.subscribers, delta);
    if (span > 0) {
        String d = delta == 0 ? String(TR("Sin cambios", "No change")) : (delta > 0 ? "+" : "-") + fmtCompact(delta < 0 ? -delta : delta);
        pill(170, d + (span == 1 ? String(TR(" desde ayer", " since yesterday"))
                                  : TR(" en ", " in ") + String(span) + TR(" días", " days")),
             delta > 0 ? C_UP : delta < 0 ? C_DOWN : C_MUTED, delta > 0 ? 1 : delta < 0 ? -1 : 0, TOUCH_SUBSCRIBERS, 0,
             L_SUBSCRIBERS);
        if (!(learned & L_SUBSCRIBERS)) hint(TR("Toca el recuadro: últimos 30 días", "Tap the box: last 30 days"));
    }
}

static void drawViews(const ChannelStats &st, int32_t today)
{
    label(70, TR("VISTAS TOTALES", "TOTAL VIEWS"));
    bigNumber(124, fmtInt(st.views), fmtCompact(st.views));
    int64_t todayViews[1];
    historyDailyViews(today, 1, todayViews);
    if (todayViews[0] >= 0) {
        pill(170, "+" + fmtCompact(todayViews[0]) + TR(" hoy", " today"), todayViews[0] > 0 ? C_UP : C_MUTED, todayViews[0] > 0, TOUCH_DAY, 13,
             L_DAY);
        if (!(learned & L_DAY)) hint(TR("Toca el recuadro: vistas de cada día", "Tap the box: views per day"));
    }
}

// Cifra con su nombre debajo, en un recuadro; con borde rojo si se puede tocar
static void statChip(int x, int y, int w, const String &value, const String &name, bool touchable)
{
    roundBox(x, y, w, 54, 10, C_PANEL);
    if (touchable) touchBorder(x, y, w, 54, 10, L_COMMENTS);
    font(FontStrong);
    text(x + w / 2, y + 27, value, C_TEXT, 1);
    font(FontSmall);
    text(x + w / 2, y + 45, name, touchable ? C_RED : C_MUTED, 1);
}

static void drawVideo(const ChannelStats &st, time_t now)
{
    font(FontLabel);
    text(16, 66, TR("ÚLTIMO VÍDEO", "LATEST VIDEO"), C_MUTED);
    text(W - 16, 66, fmtAgo(st.videoPublished, now), C_MUTED, 2);
    if (!st.hasVideo) {
        font(FontStrong);
        text(W / 2, 135, TR("Sin vídeos públicos", "No public videos"), C_MUTED, 1);
        return;
    }
    // Título en una tarjeta tocable: abre los últimos vídeos
    roundBox(10, 72, W - 20, 62, 10, C_PANEL);
    touchBorder(10, 72, W - 20, 62, 10, L_VIDEOS);
    font(FontStrong);
    String lines[2];
    int n = wrap(st.videoTitle, W - 62, 2, lines);
    for (int i = 0; i < n; i++) text(20, (n == 1 ? 111 : 98) + i * 26, lines[i], C_TEXT);
    text(W - 22, 112, "›", C_RED, 2);
    zone(0, 44, W, 98, TOUCH_VIDEOS);

    const int y = 142, gap = 8, w = (W - 32 - 2 * gap) / 3;
    statChip(16, y, w, fmtCompact(st.videoViews), TR("vistas", "views"), false);
    statChip(16 + w + gap, y, w, st.videoLikes >= 0 ? fmtCompact(st.videoLikes) : String("—"), TR("me gusta", "likes"), false);
    bool comments = st.videoComments > 0;
    statChip(16 + 2 * (w + gap), y, w, st.videoComments >= 0 ? fmtCompact(st.videoComments) : String("—"),
             st.newComments > 0 ? String(st.newComments) + TR(" nuevo", " new") + (st.newComments == 1 || langEn() ? "" : "s")
                                 : String(TR("comentarios", "comments")),
             comments);
    if (st.newComments > 0) cv->fillSmoothCircle(W - 18, y + 2 - oy, 6, C_RED);
    if (comments) zone(16 + 2 * (w + gap) - 6, y - 8, w + 12, 74, TOUCH_COMMENTS);
    bool knowsVideos = learned & L_VIDEOS, knowsComments = (learned & L_COMMENTS) || !comments;
    if (!knowsVideos && !knowsComments) hint(TR("Toca el título o los comentarios", "Tap the title or the comments"));
    else if (!knowsVideos) hint(TR("Toca el título: tus últimos vídeos", "Tap the title: your latest videos"));
    else if (!knowsComments) hint(TR("Toca los comentarios para leerlos", "Tap the comments to read them"));
}

// "+2,4 K en 24 h" / "+120 en 3 h"
static String gainText(int64_t gain, int hours)
{
    return "+" + fmtCompact(gain) + TR(" en ", " in ") + String(hours) + " h";
}

static String decimal(float v)  // "3,2" / "3.2"
{
    String t = String(v, 1);
    if (!langEn()) t.replace(".", ",");
    return t;
}

static String timesText(float times)  // "x3,2"
{
    String t = decimal(times);
    if (t.endsWith(",0") || t.endsWith(".0")) t.remove(t.length() - 2);
    return "x" + t;
}

static void drawTrend(const ChannelStats &st)
{
    font(FontLabel);
    text(16, 66, TR("EL QUE MÁS CRECE", "FASTEST GROWING"), C_MUTED);
    if (!st.hasTrend) {
        font(FontBody);
        text(W / 2, 124, TR("Compara cuántas vistas gana", "Compares the views gained by"), C_MUTED, 1);
        text(W / 2, 148, TR("cada uno de tus 10 últimos vídeos.", "each of your 10 latest videos."), C_MUTED, 1);
        text(W / 2, 172, TR("Aparece en 1 hora.", "Shows up in 1 hour."), C_MUTED, 1);
        return;
    }
    text(W - 16, 66, TR("últimas ", "last ") + String(st.trendHours) + " h", C_MUTED, 2);
    // Título en tarjeta tocable: abre tus vídeos ordenados por lo que más crecen
    roundBox(10, 72, W - 20, 62, 10, C_PANEL);
    touchBorder(10, 72, W - 20, 62, 10, L_VIDEOS);
    font(FontStrong);
    String lines[2];
    int n = wrap(st.trendTitle, W - 62, 2, lines);
    for (int i = 0; i < n; i++) text(20, (n == 1 ? 111 : 98) + i * 26, lines[i], C_TEXT);
    text(W - 22, 112, "›", C_RED, 2);
    zone(0, 44, W, 98, TOUCH_VIDEOS, 1);
    // Cifra de lo ganado y comparación
    font(FontValue);
    String gain = "+" + fmtInt(st.trendGain);
    int gw = width(gain);
    text(16, 176, gain, C_UP);
    font(FontBody);
    text(16 + gw + 10, 159, TR("vistas", "views"), C_MUTED);
    text(16 + gw + 10, 178, TR("en ", "in ") + String(st.trendHours) + " h", C_MUTED);
    // Debajo: frente a los demás vídeos e interacción
    font(FontSmall);
    int x = 16;
    if (st.trendTimes >= 1.5f) {
        String t = timesText(st.trendTimes) + TR(" tu media", " your average");
        text(x, 200, t, C_UP);
        x += width(t);
    }
    if (st.trendEngagement >= 0) {
        String e = decimal(st.trendEngagement);
        text(x, 200, (x > 16 ? "  ·  " : "") + e + TR(" % interacción", " % engagement"), C_MUTED);
    }
    if (!(learned & L_VIDEOS)) hint(TR("Toca el título: tus 10 últimos vídeos", "Tap the title: your 10 latest videos"));
}

static const char *const WEEKDAY_SHORT[] = {"J", "V", "S", "D", "L", "M", "X"};  // el día 0 (1-1-1970) fue jueves
static const char *const WEEKDAY_SHORT_EN[] = {"T", "F", "S", "S", "M", "T", "W"};
static int weekday(int32_t day) { return ((day % 7) + 7) % 7; }

// Vistas de los 14 últimos días en barras entre top y bottom, con la inicial
// del día debajo. 'selected' (-1: ninguno) resalta un día; cada barra se puede tocar.
static void bars(int32_t today, const int64_t *days, int64_t peak, int top, int bottom, int selected)
{
    const int N = 14, slot = 20, barW = 14, x0 = (W - N * slot) / 2 + (slot - barW) / 2;
    if (peak <= 0) peak = 1;
    box(16, bottom + 1, W - 32, 1, C_LINE);
    font(FontSmall);
    for (int i = 0; i < N; i++) {
        int x = x0 + i * slot;
        bool hi = selected < 0 ? i == N - 1 : i == selected;
        int wd = weekday(today - (N - 1 - i));
        text(x + barW / 2, bottom + 20, TR(WEEKDAY_SHORT[wd], WEEKDAY_SHORT_EN[wd]), hi ? C_TEXT : C_MUTED, 1);
        zone(x - (slot - barW) / 2, top - 12, slot, bottom - top + 40, TOUCH_DAY, i);
        if (days[i] < 0) continue;
        int h = (int)((bottom - top) * days[i] / peak);
        if (h < 2) h = 2;
        uint16_t c = selected < 0 || i == selected ? C_RED : C_RED_DIM;
        if (i == N - 1) {  // hoy, todavía parcial: solo el contorno
            cv->drawRoundRect(x, bottom - h + 1 - oy, barW, h, 3, c);
            cv->drawRoundRect(x + 1, bottom - h + 2 - oy, barW - 2, h - 2, 2, c);
        } else {
            roundBox(x, bottom - h + 1, barW, h, 3, c);
        }
    }
}

// Vistas por día de las dos últimas semanas; devuelve cuántos días tienen dato
static int dailyViews(int32_t today, int64_t *days, int64_t &peak)
{
    historyDailyViews(today, 14, days);
    peak = 0;
    int valid = 0;
    for (int i = 0; i < 14; i++)
        if (days[i] >= 0) {
            valid++;
            if (days[i] > peak) peak = days[i];
        }
    return valid;
}

static void drawChart(int32_t today)
{
    int64_t days[14], peak;
    int valid = dailyViews(today, days, peak);
    font(FontLabel);
    text(16, 66, TR("VISTAS POR DÍA", "VIEWS PER DAY"), C_MUTED);
    if (valid < 2) {
        font(FontBody);
        text(W / 2, 128, TR("El gráfico aparece tras", "The chart shows up after"), C_MUTED, 1);
        text(W / 2, 152, TR("1-2 días de uso.", "1-2 days of use."), C_MUTED, 1);
        return;
    }
    text(W - 16, 66, TR("máx ", "max ") + fmtCompact(peak), C_MUTED, 2);
    bars(today, days, peak, 82, 176, -1);
    if (!(learned & L_DAY)) hint(TR("Toca una barra para ver ese día", "Tap a bar to see that day"));
}

void screenInit()
{
    loadLearned();
    lcd.init();
    lcd.setRotation(1);  // horizontal, con el USB a la izquierda
    lcd.setBrightness(180);
    lcd.fillScreen(C_BG);
    band.setColorDepth(16);
    band.createSprite(W, BAND_H);  // se reserva ya, antes de que la WiFi fragmente la memoria
}

void screenBrightness(uint8_t percent, bool night)
{
    static int last = -1;
    int level = night ? 3 : map(constrain(percent, 5, 100), 5, 100, 12, 255);
    if (level == last) return;
    last = level;
    lcd.setBrightness(level);
}

// Última vista principal dibujada: los efectos la vuelven a dibujar con
// otros valores (cifra contando, aviso deslizándose)
static Page lastPage;
static ChannelStats lastSt;
static int32_t lastToday;
static time_t lastNow;
static bool lastStale;

// Efecto "+N": cifra intermedia y burbuja (progreso 0-1)
static bool gainOn = false;
static int64_t gainFrom, gainTo;
static uint32_t gainStart;
#define GAIN_MS 1800

// Aviso deslizante
static bool toastOn = false;
static uint32_t toastStart;
static TouchTarget toastTarget;
static String toastTitle, toastText;
#define TOAST_MS 10000
#define TOAST_SLIDE_MS 350


static float easeOut(float t) { return 1 - (1 - t) * (1 - t) * (1 - t); }

static void drawToast()
{
    uint32_t t = millis() - toastStart;
    float k = t < TOAST_SLIDE_MS                ? easeOut((float)t / TOAST_SLIDE_MS)
              : t > TOAST_MS - TOAST_SLIDE_MS ? easeOut((float)(TOAST_MS - t) / TOAST_SLIDE_MS)
                                                : 1;
    const int h = 52, y = -h + (int)((h + 4) * k);
    uint16_t accent = toastTarget == TOUCH_COMMENTS ? rgb(80, 160, 255) : C_RED;
    roundBox(8, y, W - 16, h, 10, C_PANEL);
    cv->drawRoundRect(8, y - oy, W - 16, h, 10, accent);
    roundBox(8, y, 6, h, 3, accent);
    font(FontLabel);
    text(24, y + 20, toastTitle, accent);
    font(FontSmall);
    text(W - 20, y + 20, TR("ver  ›", "view  ›"), C_MUTED, 2);
    font(FontBody);
    text(24, y + 42, ellipsize(toastText, W - 48), C_TEXT);
    zone(8, y, W - 16, h, toastTarget, TOAST_ZONE);
}

static void pageBody()
{
    ChannelStats st = lastSt;
    float g = 1;
    if (gainOn && lastPage == PAGE_SUBSCRIBERS) {
        g = (float)(millis() - gainStart) / GAIN_MS;
        if (g > 1) g = 1;
        float c = easeOut(g < 0.75f ? g / 0.75f : 1);  // la cifra termina antes que la burbuja
        st.subscribers = gainFrom + (int64_t)((gainTo - gainFrom) * c);
    }
    bubbleActive = gainOn && lastPage == PAGE_SUBSCRIBERS && g < 1;
    header(st.title.length() ? st.title : String("YouTube"), lastNow, lastStale, st.battery);
    switch (lastPage) {
    case PAGE_SUBSCRIBERS: drawSubscribers(st, lastToday); break;
    case PAGE_VIEWS: drawViews(st, lastToday); break;
    case PAGE_VIDEO: drawVideo(st, lastNow); break;
    case PAGE_TREND: drawTrend(st); break;
    case PAGE_CHART: drawChart(lastToday); break;
    default: break;
    }
    dots(lastPage);
    if (bubbleActive) {
        // "+N" verde que sube desde la cifra y se desvanece (ocupa el sitio de la etiqueta)
        int y = 96 - (int)(30 * easeOut(g));
        float fade = g < 0.6f ? 0 : (g - 0.6f) / 0.4f;
        font(FontStrong);
        text(W / 2, y, "+" + fmtCompact(gainTo - gainFrom), mix(C_UP, C_BG, fade), 1);
    }
    if (toastOn) drawToast();
}

static void presentPage()
{
    present(pageBody);
    lastWasPage = true;
}

void screenPage(Page page, const ChannelStats &st, int32_t today, time_t now, bool stale)
{
    if (page != PAGE_SUBSCRIBERS) gainOn = false;
    if (page != lastPage || !lastWasPage) {  // vista nueva: late lo que aún no se ha usado
        pulseOn = true;
        pulseStart = millis();
    }
    lastPage = page;
    lastSt = st;
    lastToday = today;
    lastNow = now;
    lastStale = stale;
    presentPage();
}

void screenGain(int64_t from, int64_t to)
{
    if (from < 0 || to <= from) return;
    gainFrom = from;
    gainTo = to;
    gainStart = millis();
    gainOn = true;
    if (lastWasPage) presentPage();
}

void screenToast(TouchTarget target, const String &title, const String &text)
{
    toastTarget = target;
    toastTitle = title;
    toastText = text;
    toastStart = millis();
    toastOn = true;
    if (lastWasPage) presentPage();
}

static void dismissToast() { toastOn = false; }

// Recuadro blanco con un código QR (el lector necesita fondo claro)
static void qr(int x, int y, int size, const String &content)
{
    roundBox(x, y, size, size, 8, C_WHITE);
    cv->qrcode(content.c_str(), x + 8, y + 8 - oy, size - 16, 2);
}

void screenSetup(const char *apName)
{
    present([&] {
        logo(12, 9);
        font(FontLabel);
        text(50, 26, TR("Configuración", "Setup"), C_TEXT);
        box(0, 40, W, 1, C_LINE);
        qr(14, 56, 124, String("WIFI:T:nopass;S:") + apName + ";;");
        font(FontBody);
        text(152, 76, TR("1. Escanea el QR o", "1. Scan the QR or"), C_TEXT);
        text(152, 99, TR("conecta el móvil a", "join your phone to"), C_TEXT);
        font(FontStrong);
        text(152, 128, ellipsize(apName, W - 160), C_RED);
        font(FontBody);
        text(152, 160, TR("2. Sigue los pasos", "2. Follow the steps"), C_TEXT);
        text(152, 183, TR("que aparecen.", "that appear."), C_TEXT);
        font(FontSmall);
        text(14, 222, TR("Si no se abre solo, entra en 192.168.4.1", "If nothing opens, go to 192.168.4.1"), C_MUTED);
    });
}

void screenInfo(const String &host, const String &ip)
{
    present([&] {
        logo(12, 9);
        font(FontLabel);
        text(50, 26, TR("Ajustes", "Settings"), C_TEXT);
        box(0, 40, W, 1, C_LINE);
        qr(14, 56, 124, "http://" + ip + "/");
        font(FontBody);
        text(152, 76, TR("Escanea el QR o", "Scan the QR"), C_TEXT);
        text(152, 99, TR("abre en el móvil:", "or open:"), C_TEXT);
        font(FontLabel);
        text(152, 130, ellipsize(host + ".local", W - 160), C_TEXT);
        font(FontSmall);
        text(152, 154, TR("o bien:", "or:"), C_MUTED);
        font(FontLabel);
        text(152, 178, ip, C_TEXT);
        font(FontSmall);
        text(14, 222, TR("Toca la pantalla para ver los datos", "Tap the screen to see your stats"), C_MUTED);
    });
}

void screenMessage(const String &title, const String &line1, const String &line2)
{
    present([&] {
        logo(12, 9);
        font(FontLabel);
        text(50, 26, ellipsize(title, W - 62), C_TEXT);
        box(0, 40, W, 1, C_LINE);
        font(FontStrong);
        String lines[4];
        int maxLines = line2.length() ? 3 : 4;
        int n = wrap(line1, W - 32, maxLines, lines);
        int y = 92;
        for (int i = 0; i < n; i++, y += 30) text(16, y, lines[i], C_TEXT);
        if (line2.length()) {
            font(FontBody);
            text(16, y + 8, ellipsize(line2, W - 32), C_MUTED);
        }
    });
}

void screenProgress(const String &title, const String &line, int percent)
{
    percent = constrain(percent, 0, 100);
    present([&] {
        logo(12, 9);
        font(FontLabel);
        text(50, 26, ellipsize(title, W - 62), C_TEXT);
        box(0, 40, W, 1, C_LINE);
        font(FontValue);
        text(W / 2, 118, String(percent) + " %", C_TEXT, 1);
        roundBox(24, 140, W - 48, 16, 8, C_PANEL);
        if (percent > 0) roundBox(24, 140, (W - 48) * percent / 100 < 16 ? 16 : (W - 48) * percent / 100, 16, 8, C_RED);
        font(FontBody);
        text(W / 2, 192, line, C_MUTED, 1);
    });
}

// Un toque cuenta al apoyar el dedo; se ignora durante un momento para que
// un solo toque no cuente varias veces
// --- Táctil XPT2046 ---------------------------------------------------------
// Se lee por software en sus propios pines: el bus SPI por hardware que le
// correspondería (VSPI) lo usa la tarjeta microSD. Es un chip lento, así que
// no se nota. Mismo método que LovyanGFX: 7 lecturas de X, Y y presión, y la
// mediana de cada una.
//
// Calibrado con toques en las esquinas de una FNK0114F: valores en bruto en
// los bordes del panel (vertical, 240x320). Para otro panel: compila con
// -DCYD_TOUCH_DEBUG, toca las esquinas y mira el monitor serie.
#define TOUCH_X_MIN 186   // bruto X -> 0..239 del panel vertical
#define TOUCH_X_MAX 3898
#define TOUCH_Y_MIN 3796  // bruto Y -> 0..319 (invertido)
#define TOUCH_Y_MAX 136

#ifndef CYD_PREVIEW
static uint8_t touchTransfer(uint8_t out)
{
    uint8_t in = 0;
    for (int bit = 7; bit >= 0; bit--) {  // modo SPI 0, primero el bit más alto
        digitalWrite(TOUCH_MOSI, (out >> bit) & 1);
        delayMicroseconds(1);
        digitalWrite(TOUCH_SCLK, HIGH);
        delayMicroseconds(1);
        in = (in << 1) | digitalRead(TOUCH_MISO);
        digitalWrite(TOUCH_SCLK, LOW);
    }
    return in;
}

// Lectura en bruto; false si no se está tocando
static bool readTouchRaw(int &rx, int &ry)
{
    static bool pinsReady = false;
    if (!pinsReady) {
        pinMode(TOUCH_SCLK, OUTPUT);
        pinMode(TOUCH_MOSI, OUTPUT);
        pinMode(TOUCH_MISO, INPUT);
        pinMode(TOUCH_CS, OUTPUT);
        digitalWrite(TOUCH_CS, HIGH);
        digitalWrite(TOUCH_SCLK, LOW);
        pinsReady = true;
    }
    uint8_t d[57];
    memset(d, 0, sizeof(d));
    for (int j = 0; j < 7; j++) {
        d[j * 8 + 0] = 0x91;  // Y
        d[j * 8 + 2] = 0xB1;  // Z1
        d[j * 8 + 4] = 0xD1;  // X
        d[j * 8 + 6] = 0xC1;  // Z2
    }
    d[56] = 0x80;  // al final, a reposo
    digitalWrite(TOUCH_CS, LOW);
    for (auto &b : d) b = touchTransfer(b);
    digitalWrite(TOUCH_CS, HIGH);

    int xt[7], yt[7], zt[7], ix = 0, iy = 0, iz = 0;
    for (int j = 0; j < 7; j++) {
        const uint8_t *v = &d[j * 8];
        int x = (v[5] << 8 | v[6]) >> 3;
        int y = (v[1] << 8 | v[2]) >> 3;
        int z = 0x3200 + y - x + (((v[3] << 8 | v[4]) - (v[7] << 8 | v[8])) >> 1);
        if (x > 128 && x <= 3968) xt[ix++] = x;
        if (y > 128 && y <= 3968) yt[iy++] = y;
        if (z > 0) zt[iz++] = z;
    }
    if (ix < 3 || iy < 3 || iz < 3) return false;
    std::sort(xt, xt + ix);
    std::sort(yt, yt + iy);
    std::sort(zt, zt + iz);
    if ((zt[iz >> 1] >> 8) == 0) return false;
    rx = xt[ix >> 1];
    ry = yt[iy >> 1];
    return true;
}

// Posición en la pantalla horizontal (320x240, USB a la izquierda)
static bool readTouch(int &x, int &y, int &rx, int &ry)
{
    if (!readTouchRaw(rx, ry)) return false;
    int px = (rx - TOUCH_X_MIN) * 239 / (TOUCH_X_MAX - TOUCH_X_MIN);  // panel vertical
    int py = (ry - TOUCH_Y_MIN) * 319 / (TOUCH_Y_MAX - TOUCH_Y_MIN);
    x = constrain(py, 0, W - 1);
    y = constrain(239 - px, 0, H - 1);
    return true;
}
#else
static bool readTouch(int &, int &, int &, int &) { return false; }  // vista previa en el PC
#endif

bool screenTouch(int &tx, int &ty)
{
    static uint32_t lastPoll = 0, lastTap = 0;
    static bool wasDown = false;
    if (millis() - lastPoll < 30) return false;
    lastPoll = millis();
    int x = 0, y = 0, rx = 0, ry = 0;
    bool down = readTouch(x, y, rx, ry);
#ifdef CYD_TOUCH_DEBUG
    if (down && !wasDown) Serial.printf("TOQUE raw=%d,%d pantalla=%d,%d\n", rx, ry, x, y);
#endif
    bool tap = down && !wasDown && millis() - lastTap > 400;
    wasDown = down;
    if (!tap) return false;
    lastTap = millis();
    tx = x;
    ty = y;
    return true;
}

TouchTarget screenHit(int x, int y, int &index)
{
    for (int i = zoneCount - 1; i >= 0; i--) {
        const Zone &z = zones[i];
        if (x >= z.x && x < z.x + z.w && y >= z.y && y < z.y + z.h) {
            index = z.index;
            if (z.index == TOAST_ZONE) dismissToast();
            else learn(z.target);
            return z.target;
        }
    }
    return TOUCH_NONE;
}

// --- Vistas de detalle -----------------------------------------------------

// Cabecera con "‹" (tocar fuera de las zonas vuelve) y un texto a la derecha
static void detailHeader(const String &title, const String &right = "")
{
    font(FontStrong);
    text(12, 29, "‹", C_RED);
    font(FontLabel);
    int rw = right.length() ? width(right) : 0;
    text(W - 12, 26, right, C_MUTED, 2);
    text(32, 26, ellipsize(title, W - 32 - rw - 22), C_TEXT);
    box(0, 40, W, 1, C_LINE);
}

// Texto centrado en varias líneas (cargando, errores, listas vacías)
static void centeredMessage(const String &msg)
{
    font(FontBody);
    String lines[4];
    int n = wrap(msg, W - 48, 4, lines);
    int y = 140 - (n - 1) * 12;
    for (int i = 0; i < n; i++) text(W / 2, y + i * 24, lines[i], C_MUTED, 1);
}

#define COMMENTS_PER_SCREEN 2

void screenComments(const ChannelStats &st, const VideoComment *c, int count, int first, time_t now, const String &status)
{
    present([&] {
        String title = st.videoComments >= 0 ? fmtInt(st.videoComments) + TR(" comentarios", " comments") : String(TR("Comentarios", "Comments"));
        int last = first + COMMENTS_PER_SCREEN < count ? first + COMMENTS_PER_SCREEN : count;
        String right = count > COMMENTS_PER_SCREEN && !status.length()
                           ? String(first + 1) + "-" + String(last) + TR(" de ", " of ") + String(count) + "  ›"
                           : String("");
        detailHeader(title, right);
        if (status.length()) {
            centeredMessage(status);
            return;
        }
        for (int i = first; i < last; i++) {
            const VideoComment &vc = c[i];
            int y = 44 + (i - first) * 98;
            if (i > first) box(16, y - 3, W - 32, 1, C_LINE);
            font(FontSmall);
            String meta = fmtAgoShort(vc.published, now);
            if (vc.likes > 0) meta = fmtCompact(vc.likes) + TR(" me gusta · ", " likes · ") + meta;
            if (vc.replies > 0) meta = String(vc.replies) + TR(" resp. · ", " repl. · ") + meta;
            int mw = width(meta);
            text(W - 16, y + 20, meta, C_MUTED, 2);
            font(FontLabel);
            int ax = 16;
            if (i < st.newComments) {  // todavía sin ver
                cv->fillSmoothCircle(20, y + 14 - oy, 4, C_RED);
                ax = 30;
            }
            text(ax, y + 20, ellipsize(vc.author, W - 16 - ax - mw - 10), C_TEXT);
            font(FontBody);
            String lines[3];
            int n = wrap(vc.text, W - 32, 3, lines);
            for (int l = 0; l < n; l++) text(16, y + 44 + l * 22, lines[l], C_TEXT);
        }
        if (count > COMMENTS_PER_SCREEN) zone(0, 41, W, H - 41, TOUCH_MORE, last < count ? last : 0);
    });
}

#define VIDEOS_PER_SCREEN 4

void screenVideos(const VideoSummary *v, int count, int first, bool byGrowth, time_t now, const String &status)
{
    // Orden: el de la API (más recientes primero) o por vistas ganadas en 24 h
    int order[TRENDS_VIDEOS], hours[TRENDS_VIDEOS];
    int64_t gain[TRENDS_VIDEOS];
    if (count > TRENDS_VIDEOS) count = TRENDS_VIDEOS;
    for (int i = 0; i < count; i++) {
        order[i] = i;
        gain[i] = trendsGain(v[i].id, now, hours[i]);
    }
    if (byGrowth) std::stable_sort(order, order + count, [&](int a, int b) { return gain[a] > gain[b]; });
    present([&] {
        int last = first + VIDEOS_PER_SCREEN < count ? first + VIDEOS_PER_SCREEN : count;
        String right = byGrowth ? TR("más crecen  ›", "growing  ›") : TR("recientes  ›", "recent  ›");
        detailHeader(TR("Tus vídeos", "Your videos"), status.length() ? String("") : right);
        if (status.length()) {
            centeredMessage(status);
            return;
        }
        zone(W / 2, 0, W / 2, 41, TOUCH_SORT);  // cambiar el orden
        const int rowH = 44;
        for (int r = first; r < last; r++) {
            const VideoSummary &vs = v[order[r]];
            int y = 44 + (r - first) * rowH;
            if (r > first) box(16, y - 2, W - 32, 1, C_LINE);
            font(FontBody);
            String ago = fmtAgoShort(vs.published, now);
            font(FontSmall);
            int aw = width(ago);
            text(W - 16, y + 19, ago, C_MUTED, 2);
            font(FontBody);
            text(16, y + 19, ellipsize(vs.title, W - 32 - aw - 10), C_TEXT);
            // Segunda línea: vistas · lo ganado (verde) · interacción
            font(FontSmall);
            int x = 16;
            String views = fmtCompact(vs.views) + TR(" vistas", " views");
            text(x, y + 39, views, C_MUTED);
            x += width(views);
            int64_t g = gain[order[r]];
            if (g >= 0) {
                String gt = "  ·  " + gainText(g, hours[order[r]]);
                text(x, y + 39, gt, g > 0 ? C_UP : C_MUTED);
                x += width(gt);
            }
            float e = trendsEngagement(vs);
            if (e >= 0) {
                text(x, y + 39, "  ·  " + decimal(e) + " %", C_MUTED);
            }
        }
        if (count > VIDEOS_PER_SCREEN) {
            font(FontSmall);
            text(W - 16, 236, String(first + 1) + "-" + String(last) + TR(" de ", " of ") + String(count) + "  ›", C_MUTED, 2);
            zone(0, 42, W, H - 42, TOUCH_MORE, last < count ? last : 0);
        }
    });
}

static const char *const WEEKDAY_NAME[] = {"Jueves", "Viernes", "Sábado", "Domingo", "Lunes", "Martes", "Miércoles"};
static const char *const MONTH_NAME[] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio",
                                         "agosto", "septiembre", "octubre", "noviembre", "diciembre"};
static const char *const WEEKDAY_NAME_EN[] = {"Thursday", "Friday", "Saturday", "Sunday", "Monday", "Tuesday", "Wednesday"};
static const char *const MONTH_NAME_EN[] = {"January", "February", "March", "April", "May", "June", "July",
                                            "August", "September", "October", "November", "December"};

void screenDay(int32_t today, int index)
{
    present([&] {
        detailHeader(TR("Vistas por día", "Views per day"));
        int64_t days[14], peak;
        dailyViews(today, days, peak);
        int32_t day = today - (13 - index);
        time_t t = (time_t)day * 86400;
        struct tm tm;
        gmtime_r(&t, &tm);
        String when = index == 13 ? String(TR("Hoy, hasta ahora", "Today, so far"))
                      : index == 12 ? String(TR("Ayer", "Yesterday"))
                                    : String(TR(WEEKDAY_NAME[weekday(day)], WEEKDAY_NAME_EN[weekday(day)]));
        when += langEn() ? " · " + String(MONTH_NAME_EN[tm.tm_mon]) + " " + String(tm.tm_mday)
                         : " · " + String(tm.tm_mday) + " de " + MONTH_NAME[tm.tm_mon];
        font(FontLabel);
        text(16, 66, when, C_MUTED);
        int64_t v = days[index];
        if (v < 0) {
            font(FontBody);
            text(16, 102, TR("Sin datos de ese día", "No data for that day"), C_MUTED);
        } else {
            font(FontValue);
            String value = fmtInt(v);
            text(16, 106, value, C_TEXT);
            int vx = 16 + width(value) + 10;
            font(FontBody);
            text(vx, 86, TR("vistas", "views"), C_MUTED);
            // Comparación con la media de los días completos (sin hoy)
            int64_t sum = 0;
            int n = 0;
            for (int i = 0; i < 13; i++)
                if (days[i] >= 0) {
                    sum += days[i];
                    n++;
                }
            if (index != 13 && n >= 2 && sum > 0) {
                long pct = lround(100.0 * v * n / sum) - 100;
                String cmp = pct == 0 ? String(TR("en la media", "about average"))
                             : pct > 0 ? "+" + String(pct) + TR(" % sobre la media", " % above average")
                                       : String(pct) + TR(" % bajo la media", " % below average");
                font(FontSmall);
                if (vx + width(cmp) > W - 12) cmp = (pct > 0 ? "+" : "") + String(pct) + " %";  // cifras muy largas
                text(vx, 106, cmp, pct > 0 ? C_UP : pct < 0 ? C_DOWN : C_MUTED);
            }
        }
        bars(today, days, peak, 124, 196, index);
    });
}

void screenSubscriberHistory(const ChannelStats &st, int32_t today)
{
    present([&] {
        detailHeader(TR("Suscriptores · 30 días", "Subscribers · 30 days"));
        const int N = 30;
        int64_t v[N];
        historySubscribers(today, N, v);
        if (st.subscribers >= 0 && !st.subscribersHidden) v[N - 1] = st.subscribers;  // el dato de ahora
        int first = -1, valid = 0;
        int64_t lo = 0, hi = 0;
        for (int i = 0; i < N; i++) {
            if (v[i] < 0) continue;
            if (first < 0) {
                first = i;
                lo = hi = v[i];
            }
            if (v[i] < lo) lo = v[i];
            if (v[i] > hi) hi = v[i];
            valid++;
        }
        if (valid < 2) {
            centeredMessage(TR("La evolución aparece tras unos días de uso.", "The trend shows up after a few days of use."));
            return;
        }
        font(FontValue);
        text(16, 96, fmtInt(v[N - 1] >= 0 ? v[N - 1] : hi), C_TEXT);
        int last = N - 1;
        while (v[last] < 0) last--;
        int64_t delta = v[last] - v[first];
        int span = last - first;
        String d = (delta > 0 ? "+" : delta < 0 ? "-" : "") + fmtCompact(delta < 0 ? -delta : delta) + TR(" en ", " in ") +
                   String(span) + (span == 1 ? TR(" día", " day") : TR(" días", " days"));
        font(FontSmall);
        text(W - 16, 96, d, delta > 0 ? C_UP : delta < 0 ? C_DOWN : C_MUTED, 2);

        // Línea con relleno; los días sin dato se saltan
        const int left = 16, right = W - 16, top = 116, bottom = 192;
        if (hi == lo) hi = lo + 1;
        auto px = [&](int i) { return left + (right - left) * i / (N - 1); };
        auto py = [&](int64_t val) { return bottom - (int)((bottom - top) * (val - lo) / (hi - lo)); };
        for (int i = first, prev = -1; i < N; i++) {
            if (v[i] < 0) continue;
            if (prev >= 0) {
                int x0 = px(prev), y0 = py(v[prev]), x1 = px(i), y1 = py(v[i]);
                cv->fillTriangle(x0, y0 - oy, x1, y1 - oy, x0, bottom - oy, C_RED_DIM);
                cv->fillTriangle(x1, y1 - oy, x1, bottom - oy, x0, bottom - oy, C_RED_DIM);
            }
            prev = i;
        }
        for (int i = first, prev = -1; i < N; i++) {
            if (v[i] < 0) continue;
            if (prev >= 0) cv->drawWideLine(px(prev), py(v[prev]) - oy, px(i), py(v[i]) - oy, 1.5f, C_RED);
            prev = i;
        }
        cv->fillSmoothCircle(px(last), py(v[last]) - oy, 4, C_RED);
        box(left, bottom + 1, right - left, 1, C_LINE);
        font(FontSmall);
        text(left, 214, langEn() ? String(N - 1) + " d ago" : "hace " + String(N - 1) + " d", C_MUTED);
        text(right, 214, TR("hoy", "today"), C_MUTED, 2);
    });
}

// --- Celebración de un hito -----------------------------------------------

// Confeti que cae alrededor de la tarjeta central. Se dibuja directamente en
// la pantalla, trozo a trozo, sin redibujar el resto: así va fluido. Los
// trozos que tocarían la tarjeta no se dibujan (pasan "por detrás").
#define CARD_X 22
#define CARD_Y 62
#define CARD_W (W - 2 * CARD_X)
#define CARD_H 150
#define CONFETTI_N 48
#define CONFETTI_MS 7000

struct Piece {
    float x, y, vx, vy;
    uint8_t w, h, color;
    int16_t dx, dy;  // dónde está dibujado (para borrarlo)
    bool drawn;
};
static Piece pieces[CONFETTI_N];
static bool confettiOn = false;
static uint32_t confettiStart, confettiLast;
static uint32_t rngState = 1;

static int rnd(int n)
{
    rngState = rngState * 1103515245 + 12345;
    return (int)((rngState >> 16) % n);
}

static const uint16_t CONFETTI_COLORS[] = {C_RED, C_UP, C_WARN, rgb(80, 160, 255), rgb(255, 120, 200), C_WHITE};

static void spawn(Piece &p, bool anywhere)
{
    p.w = 3 + rnd(4);
    p.h = 4 + rnd(7);
    p.x = rnd(W);
    p.y = anywhere ? rnd(H + 80) - 80 : -p.h - rnd(60);
    p.vx = (rnd(41) - 20) / 400.0f;  // px/ms
    p.vy = (40 + rnd(60)) / 1000.0f;
    p.color = rnd(6);
}

static bool overCard(int x, int y, int w, int h)
{
    return x + w > CARD_X - 2 && x < CARD_X + CARD_W + 2 && y + h > CARD_Y - 2 && y < CARD_Y + CARD_H + 2;
}

static void stopConfetti() { confettiOn = false; }

void screenCelebrate(const String &value, const String &what)
{
    present([&] {
        roundBox(CARD_X, CARD_Y, CARD_W, CARD_H, 16, C_PANEL);
        font(FontStrong);
        text(W / 2, 98, TR("¡Enhorabuena!", "Congratulations!"), C_TEXT, 1);
        bigNumber(140, value, value, CARD_W - 28);
        font(FontStrong);
        text(W / 2, 196, what, C_RED, 1);
    });
    rngState = millis() | 1;
    for (auto &p : pieces) {
        spawn(p, true);
        p.drawn = false;
    }
    confettiStart = confettiLast = millis();
    confettiOn = true;
}

static void animateConfetti()
{
    uint32_t now = millis();
    if (now - confettiLast < 25) return;  // ~40 imágenes por segundo
    float dt = now - confettiLast;
    confettiLast = now;
    bool more = now - confettiStart < CONFETTI_MS - 2500;  // los últimos segundos ya no salen nuevos
    bool any = false;
    lcd.startWrite();
    for (auto &p : pieces) {
        if (p.drawn) lcd.fillRect(p.dx, p.dy, p.w, p.h, C_BG);
        p.drawn = false;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vx += (rnd(21) - 10) / 20000.0f * dt;  // vaivén
        if (p.y > H) {
            if (!more) continue;
            spawn(p, false);
        }
        any = true;
        int x = (int)p.x, y = (int)p.y;
        if (y + p.h <= 0 || overCard(x, y, p.w, p.h)) continue;
        // Gira: alterna ancho y alto de vez en cuando
        if (rnd(30) == 0) std::swap(p.w, p.h);
        lcd.fillRect(x, y, p.w, p.h, CONFETTI_COLORS[p.color]);
        p.dx = x;
        p.dy = y;
        p.drawn = true;
    }
    lcd.endWrite();
    if (!any || now - confettiStart > CONFETTI_MS) {
        lcd.startWrite();
        for (auto &p : pieces)
            if (p.drawn) lcd.fillRect(p.dx, p.dy, p.w, p.h, C_BG);
        lcd.endWrite();
        confettiOn = false;
    }
}

void screenAnimate()
{
    if (confettiOn) animateConfetti();
    if (!lastWasPage) return;
    static uint32_t lastFrame = 0;
    uint32_t now = millis();
    bool gainRunning = gainOn && lastPage == PAGE_SUBSCRIBERS && now - gainStart < GAIN_MS + 100;
    uint32_t tt = now - toastStart;
    bool toastMoving = toastOn && (tt < TOAST_SLIDE_MS + 60 || tt > TOAST_MS - TOAST_SLIDE_MS);
    if (toastOn && tt >= TOAST_MS) {
        toastOn = false;
        presentPage();  // sin el aviso
        return;
    }
    if (gainOn && !gainRunning) {
        gainOn = false;
        presentPage();  // imagen final, sin la burbuja
        return;
    }
    bool pulsing = pulseOn && now - pulseStart < PULSE_MS;
    if (pulseOn && !pulsing) {
        pulseOn = false;
        presentPage();  // borde en reposo
        return;
    }
    if ((gainRunning || toastMoving || pulsing) && now - lastFrame >= 30) {
        lastFrame = now;
        presentPage();
    }
}

#endif
