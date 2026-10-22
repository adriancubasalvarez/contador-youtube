// Solo para la versión OLED (la que se compila si no se elige otra en config.h).
#include "../../config.h"
#if !defined(LILYGO_T5_EPD47_S3) && !defined(BOARD_CYD)

#include "../screen.h"
#include <U8g2lib.h>
#include <Wire.h>
#include "../format.h"
#include "../history.h"
#include "../lang.h"

// SSD1306 128x64 de dos colores: filas 0-15 amarillas (cabecera) y 16-63
// azules (contenido).

#define OLED_SDA 21
#define OLED_SCL 22
#define W 128
#define H 64
#define HEADER_H 16  // franja amarilla

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// Desplazamiento de 1 px que cambia cada minuto, para que el OLED no se
// "queme" con las cifras fijas.
static int jx = 0;

// Fuentes grandes de mayor a menor; se usa la mayor en la que quepa la cifra
static const uint8_t *BIG_FONTS[] = {u8g2_font_logisoso32_tr, u8g2_font_logisoso28_tr, u8g2_font_logisoso24_tr,
                                     u8g2_font_logisoso20_tr, u8g2_font_logisoso16_tr};

// Caracteres que no existen en las fuentes Latin-1 del OLED
static String plain(String s)
{
    s.replace("—", "-");
    s.replace("…", "...");
    s.replace("·", "-");
    return s;
}

static int width(const String &s) { return oled.getUTF8Width(s.c_str()); }

static void text(int x, int y, const String &s) { oled.drawUTF8(x, y, s.c_str()); }

static void textRight(int x, int y, const String &s) { oled.drawUTF8(x - width(s), y, s.c_str()); }

static String ellipsize(String s, int maxW)
{
    if (width(s) <= maxW) return s;
    while (s.length() && width(s + "...") > maxW) {
        int i = s.length() - 1;
        while (i > 0 && ((uint8_t)s[i] & 0xC0) == 0x80) i--;  // respeta UTF-8
        s.remove(i);
    }
    s.trim();
    return s + "...";
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

static void header(const String &left, const String &right)
{
    oled.setFont(u8g2_font_helvB08_tf);
    int leftW = width(left);
    text(0, 11, left);
    oled.setFont(u8g2_font_helvR08_tf);
    if (right.length()) textRight(W, 11, ellipsize(plain(right), W - leftW - 6));
}

// Cifra grande centrada en la zona azul
static void bigNumber(const String &full, const String &compact)
{
    for (const String &value : {plain(full), plain(compact)}) {
        for (const uint8_t *font : BIG_FONTS) {
            oled.setFont(font);
            int w = width(value);
            if (w <= W - 4) {
                int h = oled.getAscent();
                text((W - w) / 2 + jx, 40 + h / 2, value);
                return;
            }
        }
    }
}

void screenInit()
{
    Wire.begin(OLED_SDA, OLED_SCL);
    oled.begin();
    oled.enableUTF8Print();
    oled.setFontPosBaseline();
}

// El contraste del SSD1306 apenas cambia el brillo por debajo de ~30 %, así
// que para los valores bajos se reducen además la precarga y el VCOMH.
void screenBrightness(uint8_t percent, bool night)
{
    static int last = -1;
    int level = night ? 0 : constrain(percent, 5, 100);
    if (level == last) return;
    last = level;
    bool dim = level <= 25;
    oled.sendF("ca", 0xD9, dim ? 0x11 : 0xF1);  // precarga (valor de fábrica: 0xF1)
    oled.sendF("ca", 0xDB, dim ? 0x00 : 0x40);  // VCOMH (valor de fábrica: 0x40)
    oled.setContrast(level == 0 ? 0 : map(level, 5, 100, 1, 255));
}

void screenPage(Page page, const ChannelStats &st, int32_t today, time_t now, bool stale)
{
    jx = (int)((millis() / 60000) % 3) - 1;
    String flag = stale ? "! " : "";
    oled.clearBuffer();

    switch (page) {
    case PAGE_SUBSCRIBERS: {
        int64_t delta = 0;
        int span = historySubscriberDelta(today, 7, st.subscribers, delta);
        String right;
        if (span > 0 && !st.subscribersHidden)
            right = (delta > 0 ? "+" : delta < 0 ? "-" : "") + fmtCompact(delta < 0 ? -delta : delta) + " " +
                    String(span) + "d";
        header(TR("SUSCRIPTORES", "SUBSCRIBERS"), flag + right);
        if (st.subscribersHidden) {
            oled.setFont(u8g2_font_helvR08_tf);
            text(jx + 2, 44, TR("Contador oculto", "Count hidden"));
        } else {
            bigNumber(fmtInt(st.subscribers), fmtCompact(st.subscribers));
        }
        break;
    }
    case PAGE_VIEWS: {
        int64_t todayViews[1];
        historyDailyViews(today, 1, todayViews);
        header(TR("VISTAS", "VIEWS"), flag + (todayViews[0] >= 0 ? TR("hoy +", "today +") + fmtCompact(todayViews[0]) : String("")));
        bigNumber(fmtInt(st.views), fmtCompact(st.views));
        break;
    }
    case PAGE_VIDEO: {
        header(TR("ÚLTIMO VÍDEO", "LATEST VIDEO"), flag + fmtAgoShort(st.videoPublished, now));
        oled.setFont(u8g2_font_helvR08_tf);
        if (!st.hasVideo) {
            text(jx + 2, 44, TR("Sin vídeos públicos", "No public videos"));
            break;
        }
        String lines[2];
        int n = wrap(plain(st.videoTitle), W - 2, 2, lines);
        for (int i = 0; i < n; i++) text(jx + 1, 27 + i * 11, lines[i]);
        oled.setFont(u8g2_font_helvB12_tf);
        String views = plain(fmtCompact(st.videoViews));
        int vw = width(views);
        text(jx + 1, 62, views);
        oled.setFont(u8g2_font_helvR08_tf);
        text(jx + 1 + vw + 4, 62, TR("vistas", "views"));
        break;
    }
    case PAGE_TREND: {
        header(TR("EL QUE MÁS CRECE", "FASTEST GROWING"), flag + (st.hasTrend ? String(st.trendHours) + " h" : String("")));
        oled.setFont(u8g2_font_helvR08_tf);
        if (!st.hasTrend) {
            text(jx + 2, 36, TR("Compara tus 10 últimos", "Compares your 10 latest"));
            text(jx + 2, 48, TR("vídeos: aparece en 1 hora.", "videos: ready in 1 hour."));
            break;
        }
        String lines[2];
        int n = wrap(plain(st.trendTitle), W - 2, 2, lines);
        for (int i = 0; i < n; i++) text(jx + 1, 27 + i * 11, lines[i]);
        oled.setFont(u8g2_font_helvB12_tf);
        String gain = "+" + plain(fmtCompact(st.trendGain));
        int gw = width(gain);
        text(jx + 1, 62, gain);
        oled.setFont(u8g2_font_helvR08_tf);
        String times = String(st.trendTimes, 1);
        if (!langEn()) times.replace(".", ",");
        text(jx + 1 + gw + 4, 62, st.trendTimes >= 1.5f ? "x" + times + TR(" tu media", " your avg") : String(TR("vistas", "views")));
        break;
    }
    case PAGE_CHART: {
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
        header(TR("VISTAS/DÍA", "VIEWS/DAY"), flag + (valid >= 2 ? TR("máx ", "max ") + fmtCompact(peak) : String("14 d")));
        if (valid < 2) {
            oled.setFont(u8g2_font_helvR08_tf);
            text(jx + 2, 36, TR("El gráfico aparece", "The chart shows up"));
            text(jx + 2, 48, TR("tras 1-2 días de uso.", "after 1-2 days of use."));
            break;
        }
        if (peak == 0) peak = 1;
        const int top = 19, bottom = 63, slot = 9, barW = 7, x0 = (W - N * slot) / 2 + 1 + jx;
        for (int i = 0; i < N; i++) {
            int x = x0 + i * slot;
            if (days[i] < 0) {
                oled.drawHLine(x, bottom, barW);
                continue;
            }
            int h = (int)((bottom - top) * days[i] / peak);
            if (h < 1) h = 1;
            if (i == N - 1)
                oled.drawFrame(x, bottom - h + 1, barW, h);  // hoy, todavía parcial: solo el contorno
            else
                oled.drawBox(x, bottom - h + 1, barW, h);
        }
        break;
    }
    default:
        break;
    }
    oled.sendBuffer();
}

void screenSetup(const char *apName)
{
    oled.clearBuffer();
    header(TR("CONFIGURACIÓN", "SETUP"), "");
    oled.setFont(u8g2_font_helvR08_tf);
    text(0, 27, TR("1. Conecta el móvil a:", "1. Join your phone to:"));
    oled.setFont(u8g2_font_helvB10_tf);
    text(8, 41, apName);
    oled.setFont(u8g2_font_helvR08_tf);
    text(0, 52, TR("2. Sigue los pasos", "2. Follow the steps"));
    text(0, 63, TR("(o abre 192.168.4.1)", "(or open 192.168.4.1)"));
    oled.sendBuffer();
}

void screenInfo(const String &host, const String &ip)
{
    oled.clearBuffer();
    header(TR("AJUSTES", "SETTINGS"), "");
    oled.setFont(u8g2_font_helvR08_tf);
    text(0, 27, TR("Abre en el navegador:", "Open in your browser:"));
    oled.setFont(u8g2_font_helvB08_tf);
    text(0, 40, ellipsize(host + ".local", W));
    oled.setFont(u8g2_font_helvR08_tf);
    text(0, 52, TR("o bien:", "or:"));
    oled.setFont(u8g2_font_helvB08_tf);
    text(0, 63, ip);
    oled.sendBuffer();
}

void screenMessage(const String &title, const String &line1, const String &line2)
{
    oled.clearBuffer();
    header(plain(title), "");
    oled.setFont(u8g2_font_helvR08_tf);
    String lines[4];
    int maxLines = line2.length() ? 3 : 4;
    int n = wrap(plain(line1), W - 2, maxLines, lines);
    for (int i = 0; i < n; i++) text(1, 27 + i * 11, lines[i]);
    if (line2.length()) text(1, 27 + n * 11, ellipsize(plain(line2), W - 2));
    oled.sendBuffer();
}

void screenProgress(const String &title, const String &line, int percent)
{
    screenMessage(title, line, String(percent) + " %");
}

void screenCelebrate(const String &value, const String &what) { screenMessage(TR("¡ENHORABUENA!", "CONGRATULATIONS!"), value + " " + what); }

void screenAnimate() {}
void screenGain(int64_t, int64_t) {}
void screenToast(TouchTarget, const String &, const String &) {}

// La OLED no es táctil: no hay toques, zonas ni vistas de detalle
bool screenTouch(int &, int &) { return false; }
TouchTarget screenHit(int, int, int &) { return TOUCH_NONE; }
void screenComments(const ChannelStats &, const VideoComment *, int, int, time_t, const String &) {}
void screenVideos(const VideoSummary *, int, int, bool, time_t, const String &) {}
void screenDay(int32_t, int) {}
void screenSubscriberHistory(const ChannelStats &, int32_t) {}

#endif
