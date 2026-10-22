// Gadget de escritorio con los datos públicos de un canal, para dos pantallas:
// ESP32 + OLED 0,96" o la pantalla táctil de 2,8" con ESP32 integrado (CYD).
//
// Va siempre enchufado por USB: rota entre 4 pantallas cada pocos segundos
// y consulta YouTube cada pocos minutos.
//
// Botón BOOT (GPIO0): pulsación corta = siguiente pantalla;
// mantenerlo 3 s = abrir la configuración. En la CYD, tocar la pantalla
// también pasa a la siguiente.
//
// CÓMO CARGARLO CON ARDUINO IDE
//   1. En la pestaña config.h, elige tu pantalla.
//   2. Herramientas > Placa > esp32 > "ESP32 Dev Module".
//   3. Herramientas > Partition Scheme > "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"
//      (deja sitio para actualizar después sin cable).
//   4. Librerías (Programa > Incluir librería > Administrar bibliotecas):
//      WiFiManager (tzapu), ArduinoJson (Benoit Blanchon) y además
//      U8g2 (oliver) para la OLED o LovyanGFX (lovyan03) para la CYD.
//   5. Crea la pestaña secrets.h con tu clave (mira secrets.example.h).
//   6. Herramientas > Puerto: el COM de la placa. Pulsa "Subir" (flecha →).
//
// Conexiones de la OLED: VCC-3V3, GND-GND, SCL-GPIO22, SDA-GPIO21.
// La CYD no necesita cables: solo el USB.
#include "config.h"
#ifndef LILYGO_T5_EPD47_S3  // la versión de tinta tiene su propio programa en src/epd47

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "src/alerts.h"
#include "src/battery.h"
#include "src/format.h"
#include "src/history.h"
#include "src/portal.h"
#include "src/provision.h"
#include "src/screen.h"
#include "src/sdcard.h"
#include "src/trends.h"
#include "src/updater.h"
#include "src/settings.h"
#include "src/util.h"
#include "src/webui.h"
#include "src/youtube.h"

#define BUTTON_PIN 0
// El último vídeo se consulta aparte (2 unidades de cuota): cada 10 min basta
// para avisar pronto de los comentarios nuevos (~580 unidades al día en total: 288 del canal + 288 del vídeo)
#define VIDEO_REFRESH_MIN 10
#define RETRY_MIN 2
#define QUOTA_RETRY_MIN 60
#define LONG_PRESS_MS 3000
// Si no conecta en este tiempo, vuelve a la configuración. Es largo a propósito:
// algunos routers con PMF/WPA3 rechazan durante un rato la reconexión de un
// equipo que se reinició sin despedirse («Association refused temporarily»).
#define WIFI_TIMEOUT_MS 120000
#define WIFI_RETRY_MS 10000    // un intento nuevo cada 10 s
#define PAGE_INFO PAGE_COUNT   // pantalla con la dirección de ajustes: solo con el botón
#define INFO_SECONDS 8
// Pantalla táctil: las vistas de detalle se cierran solas si no se tocan, y
// lo que se pide a YouTube al tocar se reutiliza durante unos minutos
#define DETAIL_TIMEOUT_S 30
#define CELEBRATION_S 12
#define DETAIL_CACHE_MIN 5
#define MAX_COMMENTS 6
#define MAX_VIDEOS TRENDS_VIDEOS
// Los 10 últimos vídeos se consultan cada media hora (2 unidades: ~96 al día)
// para calcular cuánto crece cada uno; el detalle de vídeos usa esos datos
#define RECENT_REFRESH_MIN 30

static Settings settings;
static ChannelStats stats;
static bool haveData = false;
static bool stale = false;
static String fatalTitle, fatalDetail;  // error que requiere reconfigurar

static uint32_t nextFetchAt = 0, lastVideoAt = 0;
static bool videoFetched = false;
static int page = 0;
static uint32_t pageSince = 0;

enum Detail { DETAIL_NONE, DETAIL_COMMENTS, DETAIL_VIDEOS, DETAIL_DAY, DETAIL_SUBSCRIBERS, DETAIL_CELEBRATION };
static Detail detail = DETAIL_NONE;
static int detailIndex = 0;  // día elegido o primer comentario visible
static uint32_t detailTouchedAt = 0;
static String detailStatus;  // "Cargando...", error o lista vacía
static String celebrateValue, celebrateWhat;  // hito que se celebra
static int pendingAlert = -1;                 // AlertKind que se anunciará tras dibujar
static int64_t gainFrom = -1;                 // suscriptores antes de la subida
static VideoComment comments[MAX_COMMENTS];
static int commentCount = 0;
static String commentsFor;  // vídeo al que pertenecen
static uint32_t commentsAt = 0;
static VideoSummary videos[MAX_VIDEOS];
static int videoCount = 0;
static uint32_t videosAt = 0;
static bool videosByGrowth = false;  // detalle de vídeos: ordenados por lo que más crecen

// Al cambiar de canal: fuera el detalle abierto y lo guardado
static void forgetDetails()
{
    detail = DETAIL_NONE;
    commentsAt = videosAt = 0;
}

static void minutesFromNow(uint32_t minutes) { nextFetchAt = millis() + minutes * 60000UL; }

// Abre el portal. Devuelve true si se guardó y la WiFi conectó; false si se
// agotó el tiempo (10 min) o no se pudo conectar.
static bool configure()
{
    webuiStop();  // el portal usa el mismo puerto 80
    const String previousChannel = settings.channelInput;
    screenSetup(SETUP_AP_NAME);
    Serial.println("Configuración: portal abierto");
    bool ok = runSetupPortal(settings);
    Serial.printf("Configuración: %s\n", ok ? "guardada y conectada" : "sin completar");
    if (settings.channelInput != previousChannel) {
        historyClear();
        forgetDetails();
        haveData = false;
        videoFetched = false;
    }
    fatalTitle = "";
    nextFetchAt = millis();  // consultar ya
    return ok;
}

static bool connectWifi()
{
    if (WiFi.isConnected()) return true;  // p. ej. recién configurada en el portal
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);  // si luego se cae el router, vuelve solo
    uint32_t start = millis();
    int attempt = 0;
    while (millis() - start < WIFI_TIMEOUT_MS) {
        if (attempt++) {
            Serial.printf("WiFi: reintento %d\n", attempt);
            screenMessage("Dashboard YouTube", "Conectando a la WiFi...", "Intento " + String(attempt) + " de " +
                                                  String(WIFI_TIMEOUT_MS / WIFI_RETRY_MS));
            WiFi.disconnect();
            delay(300);
        }
        WiFi.begin();  // credenciales guardadas por el portal
        uint32_t t = millis();
        while (!WiFi.isConnected() && millis() - t < WIFI_RETRY_MS) delay(100);
        if (WiFi.isConnected()) return true;
    }
    return false;
}

// Conecta a la WiFi guardada. Si no lo consigue, avisa y vuelve a abrir la
// configuración; si nadie la toca, reintenta (el router podía estar apagado).
// Nombre de la red guardada (WiFi.SSID() solo lo da una vez conectado)
static String savedSsid()
{
    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) return "";
    return String((const char *)conf.sta.ssid).substring(0, sizeof(conf.sta.ssid));
}

static void ensureConnected()
{
    WiFi.mode(WIFI_STA);  // arranca la WiFi para poder leer la red guardada
    while (true) {
        String ssid = savedSsid();
        screenMessage("Dashboard YouTube", "Conectando a la WiFi " + (ssid.length() ? "«" + ssid + "»" : String("")) + "...");
        Serial.printf("WiFi: conectando a '%s'\n", ssid.c_str());
        if (connectWifi()) {
            Serial.printf("WiFi: conectado, IP %s\n", WiFi.localIP().toString().c_str());
            return;
        }
        Serial.printf("WiFi: no conecta (estado %d)\n", (int)WiFi.status());
        screenMessage("Sin conexión WiFi",
                      "No se pudo conectar a " + (ssid.length() ? "«" + ssid + "»" : String("tu WiFi")) +
                          ". Revisa la red (2,4 GHz) y la contraseña.",
                      "Abriendo la configuración...");
        delay(4000);
        WiFi.disconnect();
        configure();
    }
}

static void renderDetail(time_t now)
{
    int32_t today = clockValid() ? localDay(now) : 0;
    switch (detail) {
    case DETAIL_COMMENTS: screenComments(stats, comments, commentCount, detailIndex, now, detailStatus); break;
    case DETAIL_VIDEOS: screenVideos(videos, videoCount, detailIndex, videosByGrowth, now, detailStatus); break;
    case DETAIL_DAY: screenDay(today, detailIndex); break;
    case DETAIL_SUBSCRIBERS: screenSubscriberHistory(stats, today); break;
    case DETAIL_CELEBRATION: screenCelebrate(celebrateValue, celebrateWhat); break;
    default: break;
    }
}

static void render()
{
    time_t now = time(nullptr);
    struct tm tm;
    localtime_r(&now, &tm);
    bool night = settings.nightDim && clockValid() && tm.tm_hour < 7;
    screenBrightness(settings.brightness, night);
    alertsConfigure(settings.volume, settings.led, night);
    alertsSetUnread(stats.newComments > 0);
    alertsSetOffline(haveData && (stale || !WiFi.isConnected()));
    alertsAmbient(settings.ledMode, settings.ledColor, settings.ledLevel, settings.ledSpeed, settings.ledOfflineBlink);
    alertsSetBalance(settings.ledBalance[0], settings.ledBalance[1], settings.ledBalance[2]);
    int64_t todayViews[1] = {-1};
    if (clockValid()) historyDailyViews(localDay(now), 1, todayViews);
    alertsSetTrend(todayViews[0] < 0 ? -1 : todayViews[0] > 0 ? 1 : 0);
    stats.battery = settings.battery ? batteryPercent() : -1;

    if (fatalTitle.length()) {
        screenMessage(fatalTitle, fatalDetail, "Mantén BOOT 3 s para configurar");
    } else if (detail != DETAIL_NONE && haveData) {
        renderDetail(now);
    } else if (page == PAGE_INFO) {
        screenInfo(webuiHost(), WiFi.localIP().toString());
    } else if (!haveData) {
        screenMessage("Dashboard YouTube", WiFi.isConnected() ? "Consultando YouTube..." : "Conectando a la WiFi...");
    } else {
        screenPage((Page)page, stats, clockValid() ? localDay(now) : 0, now, stale);
    }
}

// Hitos de suscriptores que se celebran
static const int64_t MILESTONES[] = {100, 500, 1000, 5000, 10000, 25000, 50000, 100000, 250000, 500000,
                                     1000000, 2000000, 5000000, 10000000, 25000000, 50000000, 100000000};

static int64_t crossedMilestone(int64_t before, int64_t after)
{
    int64_t hit = 0;
    for (int64_t m : MILESTONES)
        if (before >= 0 && before < m && after >= m) hit = m;
    return hit;
}

// Vistas: se celebra cuando cambia la primera cifra (9.876.543 -> 10.000.000,
// 1.999.000 -> 2.000.000). Devuelve la cifra redonda alcanzada.
static int64_t crossedViewsMark(int64_t before, int64_t after)
{
    if (before < 1000 || after <= before) return 0;
    int64_t step = 1;
    while (step * 10 <= before) step *= 10;
    return after >= (before / step + 1) * step ? after / step * step : 0;
}

static void celebrate(int64_t value, const char *what)
{
    celebrateValue = fmtInt(value);
    celebrateWhat = what;
    detail = DETAIL_CELEBRATION;
    detailTouchedAt = millis();
    pendingAlert = ALERT_MILESTONE;
}

// Compara la consulta nueva con la anterior y prepara el aviso más importante
static void detectEvents(const ChannelStats &before, bool withVideo)
{
    bool sameVideo = withVideo && before.videoId.length() && stats.videoId == before.videoId;
    if (withVideo && before.videoId.length() && stats.videoId.length() && !sameVideo) {
        stats.newComments = 0;  // los nuevos eran del vídeo anterior
        pendingAlert = ALERT_VIDEO;
    } else if (sameVideo && before.videoComments >= 0 && stats.videoComments > before.videoComments) {
        stats.newComments += stats.videoComments - before.videoComments;
        commentsAt = 0;  // al abrirlos, que se pidan de nuevo
        pendingAlert = ALERT_COMMENT;
    }
    if (!stats.subscribersHidden && !before.subscribersHidden && before.subscribers >= 0 &&
        stats.subscribers > before.subscribers && pendingAlert != ALERT_VIDEO) {
        pendingAlert = ALERT_SUBSCRIBER;
        gainFrom = before.subscribers;
    }
    int64_t m = stats.subscribersHidden ? 0 : crossedMilestone(before.subscribers, stats.subscribers);
    if (m) {
        celebrate(m, "suscriptores");
    } else if ((m = crossedViewsMark(before.views, stats.views))) {
        celebrate(m, "vistas");
    }
}

static void showCommentToast(bool test)
{
    String error;
    YtResult r = WiFi.isConnected() ? youtubeComments(settings, stats.videoId, comments, MAX_COMMENTS, commentCount, error)
                                    : YtResult::NetworkError;
    if (r == YtResult::Ok) {  // quedan guardados: al tocar el aviso se abren al momento
        commentsFor = stats.videoId;
        commentsAt = millis();
    }
    String title = test                   ? String("Comentario más reciente")
                   : stats.newComments > 1 ? String(stats.newComments) + " comentarios nuevos"
                                           : String("Comentario nuevo");
    String text = r == YtResult::Ok && commentCount ? comments[0].author + ": " + comments[0].text : stats.videoTitle;
    screenToast(TOUCH_COMMENTS, title, text);
}

// Efecto en pantalla, luz y sonido de un aviso. 'test': desde la página de
// ajustes (suena aunque sea de noche)
static void announce(AlertKind kind, bool test = false)
{
    switch (kind) {
    case ALERT_MILESTONE: render(); break;  // la celebración ya es el detalle abierto
    case ALERT_SUBSCRIBER:
        if (detail == DETAIL_NONE) {
            page = PAGE_SUBSCRIBERS;
            pageSince = millis();
            render();
            screenGain(gainFrom, stats.subscribers);
        }
        break;
    case ALERT_COMMENT: showCommentToast(test); break;
    case ALERT_VIDEO: screenToast(TOUCH_VIDEOS, test ? "Tu último vídeo" : "Vídeo nuevo", stats.videoTitle); break;
    }
    if (test) alertTest(kind, settings.volume);
    else alertPlay(kind);
}

// Prueba desde la página de ajustes, con los datos reales del canal
static void simulate(AlertKind kind)
{
    if (!haveData || fatalTitle.length()) return;
    if (kind == ALERT_MILESTONE) {
        int64_t next = 0;
        for (int64_t m : MILESTONES)
            if (m > stats.subscribers) {
                next = m;
                break;
            }
        celebrate(next ? next : stats.subscribers, "suscriptores");
    }
    if (kind == ALERT_SUBSCRIBER) {
        int64_t step = 1;  // lo que sube la cifra redondeada de YouTube
        while (step * 1000 <= stats.subscribers) step *= 10;
        gainFrom = stats.subscribers - step;
    }
    announce(kind, true);
}

static void fetch()
{
    if (!WiFi.isConnected()) {
        stale = haveData;
        minutesFromNow(RETRY_MIN);
        return;
    }
    bool withVideo = !videoFetched || millis() - lastVideoAt >= VIDEO_REFRESH_MIN * 60000UL;
    String error;
    const ChannelStats before = stats;
    const bool hadData = haveData;
    YtResult r = youtubeFetch(settings, stats, error, withVideo);
    Serial.printf("YouTube: %d %s\n", (int)r, error.c_str());
    if (batteryMillivolts() >= 0) Serial.printf("Batería: %d mV (%d %%)\n", batteryMillivolts(), batteryPercent());
    switch (r) {
    case YtResult::Ok:
        if (hadData) detectEvents(before, withVideo);
        haveData = true;
        stale = false;
        if (withVideo) {
            videoFetched = true;
            lastVideoAt = millis();
        }
        if (clockValid()) {
            historyRecord(localDay(time(nullptr)), stats.subscribersHidden ? -1 : stats.subscribers, stats.views);
            sdLog(time(nullptr), stats);  // historial completo en la tarjeta, si la hay
        }
        minutesFromNow(settings.refreshMin);
        break;
    case YtResult::ChannelNotFound:
        fatalTitle = "Canal no encontrado";
        fatalDetail = settings.channelInput;
        minutesFromNow(settings.refreshMin);
        break;
    case YtResult::BadKey:
        fatalTitle = "Clave API no válida";
        fatalDetail = "Revisa la clave de la YouTube Data API v3.";
        minutesFromNow(settings.refreshMin);
        break;
    case YtResult::QuotaExceeded:
        stale = true;
        minutesFromNow(QUOTA_RETRY_MIN);
        break;
    default:
        stale = haveData;
        minutesFromNow(RETRY_MIN);
        break;
    }
}

// Devuelve true en una pulsación corta; abre la configuración si es larga
static bool pollButton()
{
    static uint32_t pressedAt = 0;
    static bool wasDown = false;
    bool down = digitalRead(BUTTON_PIN) == LOW;
    if (down && !wasDown) pressedAt = millis();
    if (down && millis() - pressedAt >= LONG_PRESS_MS) {
        while (digitalRead(BUTTON_PIN) == LOW) delay(10);
        wasDown = false;
        configure();
        ensureConnected();
        webuiBegin(&settings, &stats);
        page = PAGE_INFO;  // recién configurado: enseña dónde están los ajustes
        pageSince = millis();
        render();
        return false;
    }
    bool shortPress = !down && wasDown && millis() - pressedAt > 40;
    wasDown = down;
    return shortPress;
}

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    setenv("TZ", DEVICE_TZ, 1);
    tzset();
    screenInit();
    alertsInit();

    settingsLoad(settings);
    String previousChannel = settings.channelInput, sdSummary;
    // El instalador de Windows puede enviar la configuración por el cable USB
    provisionSerial(settings);
    // Tarjeta microSD (CYD): config.txt permite configurarlo sin el móvil
    sdBegin();
    updaterCheckSd();  // firmware.bin en la tarjeta: se instala y reinicia
    if (sdApplyConfig(settings, sdSummary)) {
        screenMessage("Configuración desde la tarjeta", "Aplicado: " + sdSummary + ".",
                      "El archivo pasa a llamarse config_aplicada.txt");
        delay(4000);
    }
    if (settings.channelInput != previousChannel) historyClear();  // canal nuevo: historial nuevo
    bool justConfigured = false;
    while (!settings.configured() || !wifiCredentialsSaved()) {
        configure();
        justConfigured = true;
    }
    ensureConnected();

    configTzTime(DEVICE_TZ, "pool.ntp.org", "time.google.com");
    uint32_t start = millis();
    while (WiFi.isConnected() && !clockValid() && millis() - start < 10000) delay(100);
    nextFetchAt = millis();

    webuiBegin(&settings, &stats);
    // Tras la configuración inicial, enseña unos segundos dónde están los
    // ajustes; en un arranque normal va directo a los datos (la pantalla de
    // ajustes sigue a mano con el botón o tocando la pantalla)
    page = justConfigured ? PAGE_INFO : 0;
    pageSince = millis();
    render();
}

// Cambios hechos desde la página de ajustes
static void applyWebChanges()
{
    static String lastChannel = settings.channelInput;
    uint8_t c = webuiTakeChanges();
    if (!c) return;
    if (c & WEB_CHANNEL) {
        if (settings.channelInput != lastChannel) {
            historyClear();
            forgetDetails();
            haveData = false;
            lastChannel = settings.channelInput;
        }
        fatalTitle = "";
        videoFetched = false;
        nextFetchAt = millis();
    }
    if (c & WEB_REFRESH) minutesFromNow(settings.refreshMin);
    render();
}

static String detailError(YtResult r, const char *what)
{
    switch (r) {
    case YtResult::CommentsDisabled: return "Los comentarios están desactivados en este vídeo.";
    case YtResult::QuotaExceeded: return "Se ha agotado la cuota diaria de la API de YouTube. Prueba mañana.";
    case YtResult::NetworkError: return "Sin conexión con YouTube. Toca para volver e inténtalo de nuevo.";
    default: return String("No se pudieron cargar ") + what + ".";
    }
}

// El vídeo reciente que más vistas gana, frente a la media de los demás
static void computeTrend()
{
    time_t now = time(nullptr);
    int best = -1, bestHours = 0, n = 0;
    int64_t bestGain = -1, sum = 0;
    for (int i = 0; i < videoCount; i++) {
        int hours;
        int64_t g = trendsGain(videos[i].id, now, hours);
        if (g < 0) continue;
        sum += g;
        n++;
        if (g > bestGain) {
            best = i;
            bestGain = g;
            bestHours = hours;
        }
    }
    stats.hasTrend = best >= 0;
    if (!stats.hasTrend) return;
    stats.trendTitle = videos[best].title;
    stats.trendGain = bestGain;
    stats.trendHours = bestHours;
    stats.trendViews = videos[best].views;
    stats.trendEngagement = trendsEngagement(videos[best]);
    float others = n > 1 ? (float)(sum - bestGain) / (n - 1) : 0;
    stats.trendTimes = others > 0 ? bestGain / others : 0;
}

// Los 10 últimos vídeos (vistas, me gusta, comentarios): para lo que más crece
// y para el detalle de vídeos
static YtResult fetchRecent(String &error)
{
    YtResult r = WiFi.isConnected() ? youtubeRecentVideos(settings, videos, MAX_VIDEOS, videoCount, error)
                                    : YtResult::NetworkError;
    Serial.printf("Vídeos: %d %s\n", (int)r, error.c_str());
    if (r == YtResult::Ok) {
        videosAt = millis();
        trendsRecord(videos, videoCount, time(nullptr));
        computeTrend();
    } else {
        videosAt = 0;
    }
    return r;
}

// Abre la vista de detalle de lo tocado. Los comentarios y los vídeos se
// piden a YouTube en ese momento (salvo que se hayan pedido hace poco).
static void openDetail(TouchTarget target, int index)
{
    detailTouchedAt = millis();
    detailIndex = target == TOUCH_DAY ? index : 0;
    detailStatus = "";
    String error;
    YtResult r = YtResult::Ok;
    switch (target) {
    case TOUCH_COMMENTS:
        detail = DETAIL_COMMENTS;
        if (commentsFor != stats.videoId || !commentsAt || millis() - commentsAt > DETAIL_CACHE_MIN * 60000UL) {
            detailStatus = "Cargando comentarios...";
            render();
            r = WiFi.isConnected() ? youtubeComments(settings, stats.videoId, comments, MAX_COMMENTS, commentCount, error)
                                   : YtResult::NetworkError;
            Serial.printf("Comentarios: %d %s\n", (int)r, error.c_str());
            commentsFor = stats.videoId;
            commentsAt = r == YtResult::Ok ? millis() : 0;
            if (r != YtResult::Ok) commentCount = 0;
        }
        detailStatus = r != YtResult::Ok ? detailError(r, "los comentarios")
                       : commentCount     ? String("")
                                          : String("Todavía no hay comentarios en este vídeo.");
        break;
    case TOUCH_VIDEOS:
        detail = DETAIL_VIDEOS;
        videosByGrowth = index == 1;  // desde "el que más crece": ordenados por lo que más crecen
        if (!videosAt || millis() - videosAt > RECENT_REFRESH_MIN * 60000UL) {
            detailStatus = "Cargando vídeos...";
            render();
            r = fetchRecent(error);
            if (r != YtResult::Ok) videoCount = 0;
        }
        detailStatus = r != YtResult::Ok ? detailError(r, "los vídeos") : videoCount ? String("") : String("Sin vídeos públicos.");
        break;
    case TOUCH_DAY: detail = DETAIL_DAY; break;
    case TOUCH_SUBSCRIBERS: detail = DETAIL_SUBSCRIBERS; break;
    default: return;
    }
    render();
}

static void closeDetail()
{
    if (detail == DETAIL_COMMENTS) stats.newComments = 0;  // ya están vistos
    detail = DETAIL_NONE;
    pageSince = millis();
}

// Toque en la pantalla: abre un detalle, actúa dentro de él o, si no hay
// nada en ese punto, pasa de vista (o cierra el detalle). Devuelve true si
// hay que pasar a la siguiente vista.
static bool handleTouch()
{
    int x, y, index = 0;
    if (!screenTouch(x, y)) return false;
    TouchTarget target = screenHit(x, y, index);
    if (detail == DETAIL_NONE) {
        if (target == TOUCH_NONE) return true;
        openDetail(target, index);
        return false;
    }
    detailTouchedAt = millis();
    if (target == TOUCH_DAY || target == TOUCH_MORE) {
        detailIndex = index;  // otro día del gráfico, o los siguientes comentarios o vídeos
    } else if (target == TOUCH_SORT) {
        videosByGrowth = !videosByGrowth;  // vídeos: recientes <-> los que más crecen
        detailIndex = 0;
    } else {
        closeDetail();
    }
    render();
    return false;
}

// Batería baja: un aviso (hasta que vuelva a cargarse por encima del 20 %)
static void checkBattery()
{
    static uint32_t lastCheck = 0;
    static bool warned = false;
    if (!settings.battery || millis() - lastCheck < 60000) return;
    lastCheck = millis();
    int pct = batteryPercent();
    if (pct < 0) return;
    if (pct > 20) warned = false;
    if (pct <= 10 && !warned) {
        warned = true;
        screenToast(TOUCH_NONE, "Batería baja (" + String(pct) + " %)", "Conecta el cable USB para cargarla.");
    }
}

void loop()
{
    webuiLoop();
    applyWebChanges();
    bool next = pollButton();
    if (handleTouch()) next = true;
    uint32_t timeout = (detail == DETAIL_CELEBRATION ? CELEBRATION_S : DETAIL_TIMEOUT_S) * 1000UL;
    if (detail != DETAIL_NONE && (next || millis() - detailTouchedAt > timeout)) {
        closeDetail();  // BOOT o el tiempo cierran el detalle y vuelve donde estaba
        next = false;
        render();
    }
    if ((int32_t)(millis() - nextFetchAt) >= 0) {
        fetch();
        if (haveData && !fatalTitle.length() && (!videosAt || millis() - videosAt > RECENT_REFRESH_MIN * 60000UL)) {
            String error;
            fetchRecent(error);
        }
        render();
        if (pendingAlert >= 0) announce((AlertKind)pendingAlert);
        pendingAlert = -1;
    }
    int test = webuiTakeTest();
    if (test >= 0) simulate((AlertKind)test);
    alertsLoop();
    screenAnimate();
    checkBattery();
#ifdef BATTERY_DEBUG  // para ajustar los límites de la batería: tensión cada 2 s
    static uint32_t lastBattery = 0;
    if (millis() - lastBattery > 2000) {
        lastBattery = millis();
        Serial.printf("Batería: %d mV (%d %%)\n", batteryMillivolts(), batteryPercent());
    }
#endif
    uint32_t shown = millis() - pageSince;
    bool autoNext = detail != DETAIL_NONE ? false
                    : page == PAGE_INFO   ? shown >= INFO_SECONDS * 1000UL
                                          : settings.pageSeconds && shown >= settings.pageSeconds * 1000UL;
    if (next || autoNext) {
        // El botón recorre también la pantalla de ajustes; la rotación automática, no
        page = next ? (page + 1) % (PAGE_COUNT + 1) : (page == PAGE_INFO ? 0 : (page + 1) % PAGE_COUNT);
        pageSince = millis();
        render();
    }
    delay(10);
}

#endif
