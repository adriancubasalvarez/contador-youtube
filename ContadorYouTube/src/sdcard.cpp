#include "sdcard.h"
#include "../config.h"

#if defined(BOARD_CYD) && !defined(LILYGO_T5_EPD47_S3)

#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include "format.h"
#include "lang.h"
#include "provision.h"

// Ranura microSD de la Freenove FNK0114F (esquema oficial): bus VSPI
#define SD_CS 5
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23

static const char *CONFIG = "/config.txt";
static const char *CONFIG_DONE = "/config_aplicada.txt";
static const char *HISTORY = "/historial.csv";
// Mismo orden que AlertKind (alerts.h)
static const char *const SOUNDS[] = {"/sonidos/comentario.wav", "/sonidos/suscriptor.wav", "/sonidos/video.wav",
                                     "/sonidos/hito.wav"};

static SPIClass sdSpi(VSPI);
static bool mounted = false;

bool sdBegin()
{
    sdSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    // Primero rápido; algunas tarjetas solo responden más despacio
    for (uint32_t freq : {20000000UL, 4000000UL, 1000000UL}) {
        mounted = SD.begin(SD_CS, sdSpi, freq) && SD.cardType() != CARD_NONE;
        if (mounted) break;
        SD.end();
        Serial.printf("Tarjeta SD: no se pudo montar a %lu kHz\n", (unsigned long)(freq / 1000));
    }
    Serial.printf("Tarjeta SD: %s\n", mounted ? sdDescribe().c_str() : "no hay (o no está en FAT32)");
    return mounted;
}

bool sdAvailable() { return mounted; }

static String sizeText(uint64_t bytes)
{
    if (bytes >= 1000000000ULL) {
        String s = String(bytes / 1e9, 1);
        if (!langEn()) s.replace(".", ",");
        return s + " GB";
    }
    if (bytes >= 1000000) return String((unsigned long)(bytes / 1000000)) + " MB";
    return String((unsigned long)((bytes + 999) / 1000)) + " KB";
}

String sdDescribe()
{
    if (!mounted) return "";
    String s = sizeText(SD.totalBytes() - SD.usedBytes()) + TR(" libres de ", " free of ") + sizeText(SD.totalBytes());
    File h = SD.open(HISTORY);
    if (h) {
        s += TR(" · historial ", " · history ") + sizeText(h.size());
        h.close();
    }
    return s;
}

// --- config.txt ------------------------------------------------------------

static void parseLine(String line, String &key, String &value)
{
    line.trim();
    key = value = "";
    if (!line.length() || line[0] == '#') return;
    int eq = line.indexOf('=');
    if (eq < 0) return;
    key = line.substring(0, eq);
    value = line.substring(eq + 1);
    key.trim();
    key.toLowerCase();
    value.trim();
}

bool sdApplyConfig(Settings &s, String &summary)
{
    if (!mounted || !SD.exists(CONFIG)) return false;
    File f = SD.open(CONFIG);
    if (!f) return false;
    String ssid, pass, channel, apiKey, language, lines;
    bool havePass = false;
    while (f.available()) {
        String line = f.readStringUntil('\n'), key, value;
        line.replace("\r", "");
        parseLine(line, key, value);
        if (key == "wifi") ssid = value;
        else if (key == "contrasena" || key == "contraseña" || key == "password") {
            pass = value;
            havePass = true;
            line = key + "=********";  // no queda en la tarjeta
        } else if (key == "canal" || key == "channel") channel = value;
        else if (key == "clave_api" || key == "api_key") apiKey = value;
        else if (key == "idioma" || key == "language") language = value;
        lines += line + "\n";
    }
    f.close();

    provisionApply(s, ssid, pass, havePass, channel, apiKey, language, summary);

    // Se renombra para no volver a aplicarlo en cada arranque
    SD.remove(CONFIG_DONE);
    File done = SD.open(CONFIG_DONE, FILE_WRITE);
    if (done) {
        done.print(TR("# Ya aplicado. Para cambiar algo, edítalo y vuelve a llamarlo config.txt\n",
                      "# Already applied. To change something, edit it and rename it back to config.txt\n"));
        done.print(lines);
        done.close();
    }
    SD.remove(CONFIG);
    Serial.printf("Tarjeta SD: config.txt aplicado (%s)\n", summary.c_str());
    return true;
}

// --- historial.csv ---------------------------------------------------------

static String csvText(const String &s)  // entre comillas, con las comillas dobladas
{
    String out = s;
    out.replace("\"", "\"\"");
    return "\"" + out + "\"";
}

static String csvNumber(int64_t n) { return n >= 0 ? String((long long)n) : String(""); }

void sdLog(time_t now, const ChannelStats &st)
{
    if (!mounted) return;
    bool isNew = !SD.exists(HISTORY);
    File f = SD.open(HISTORY, FILE_APPEND);
    if (!f) return;
    // Separador ";" para que Excel en español lo abra en columnas
    if (isNew) f.print("fecha;hora;suscriptores;vistas;videos;ultimo_video;vistas_video;me_gusta;comentarios\n");
    struct tm tm;
    localtime_r(&now, &tm);
    char when[24];
    strftime(when, sizeof(when), "%Y-%m-%d;%H:%M", &tm);
    String line = String(when) + ";" + (st.subscribersHidden ? String("") : csvNumber(st.subscribers)) + ";" +
                  csvNumber(st.views) + ";" + csvNumber(st.videos) + ";" + csvText(st.hasVideo ? st.videoTitle : "") +
                  ";" + csvNumber(st.videoViews) + ";" + csvNumber(st.videoLikes) + ";" + csvNumber(st.videoComments) +
                  "\n";
    f.print(line);
    f.close();
}

bool sdHasHistory() { return mounted && SD.exists(HISTORY); }
const char *sdHistoryPath() { return HISTORY; }

// --- sonidos ---------------------------------------------------------------

String sdSoundPath(int kind)
{
    if (!mounted || kind < 0 || kind > 3) return "";
    return SD.exists(SOUNDS[kind]) ? String(SOUNDS[kind]) : String("");
}

#else

bool sdBegin() { return false; }
bool sdAvailable() { return false; }
String sdDescribe() { return ""; }
bool sdApplyConfig(Settings &, String &) { return false; }
void sdLog(time_t, const ChannelStats &) {}
bool sdHasHistory() { return false; }
const char *sdHistoryPath() { return ""; }
String sdSoundPath(int) { return ""; }

#endif
