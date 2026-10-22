// Solo para la versión de tinta electrónica (entorno t5-epd47-s3 de PlatformIO).
#ifdef LILYGO_T5_EPD47_S3

// Dashboard YouTube (datos públicos) para LilyGo T5 4.7" ESP32-S3.
//
// Ciclo: despertar -> WiFi -> hora -> YouTube -> dibujar -> dormir.
// Entre actualizaciones el ESP32 está en reposo profundo (microamperios) y
// la tinta electrónica conserva la imagen sin consumir nada.
//
// Botón IO21: una pulsación corta actualiza al momento; mantenerlo 3 s
// abre la configuración.
#ifndef BOARD_HAS_PSRAM
#error "Activa la PSRAM (BOARD_HAS_PSRAM)"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include "display.h"
#include "../history.h"
#include "../portal.h"
#include "../settings.h"
#include "../util.h"
#include "utilities.h"
#include "../youtube.h"

#define WIFI_TIMEOUT_MS 20000
#define RETRY_MIN 5          // reintento tras un fallo de red
#define QUOTA_RETRY_MIN 60   // cuota de la API agotada
#define FAILS_BEFORE_NOTICE 3

// Sobreviven al reposo profundo
RTC_DATA_ATTR uint8_t failCount = 0;
RTC_DATA_ATTR bool noticeShown = false;

[[noreturn]] static void sleepFor(uint32_t minutes)
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_1, 0);  // el botón siempre despierta
    if (minutes) esp_sleep_enable_timer_wakeup((uint64_t)minutes * 60ULL * 1000000ULL);
    Serial.printf("Durmiendo %u min\n", (unsigned)minutes);
    Serial.flush();
    esp_deep_sleep_start();
    while (true) {}
}

static bool buttonHeld(uint32_t ms)
{
    pinMode(BUTTON_1, INPUT_PULLUP);
    uint32_t start = millis();
    while (digitalRead(BUTTON_1) == LOW) {
        if (millis() - start >= ms) return true;
        delay(20);
    }
    return false;
}

static void show(const String &title, const String &line1, const String &line2 = "")
{
    drawMessage(title, line1, line2);
    displayFlush();
}

static bool connectWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin();  // credenciales guardadas por el portal
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) delay(100);
    return WiFi.status() == WL_CONNECTED;
}

static void syncClock()
{
    configTzTime(DEVICE_TZ, "pool.ntp.org", "time.google.com");
    uint32_t start = millis();
    while (!clockValid() && millis() - start < 10000) delay(100);
}

// Un fallo transitorio no borra la pantalla (la imagen anterior sigue
// siendo útil); solo se avisa si se repite.
[[noreturn]] static void transientFailure(const String &title, const String &detail, uint32_t retryMin,
                                          uint32_t normalMin)
{
    failCount++;
    Serial.println(title + ": " + detail);
    if (failCount >= FAILS_BEFORE_NOTICE && !noticeShown) {
        show(title, detail);
        noticeShown = true;
    }
    sleepFor(failCount >= FAILS_BEFORE_NOTICE ? normalMin : retryMin);
}

void setup()
{
    Serial.begin(115200);
    setenv("TZ", DEVICE_TZ, 1);
    tzset();

    if (!displayInit()) {
        Serial.println("Sin memoria PSRAM para el framebuffer");
        sleepFor(0);
    }

    Settings s;
    settingsLoad(s);
    const String previousChannel = s.channelInput;

    bool reconfigure = buttonHeld(3000);
    if (reconfigure || !s.configured() || !wifiCredentialsSaved()) {
        drawSetupScreen(SETUP_AP_NAME);
        displayFlush();
        if (!runSetupPortal(s)) {
            show("Configuración pendiente", "No se completó la configuración.",
                 "Pulsa el botón IO21 para empezar de nuevo.");
            sleepFor(0);  // solo el botón lo despierta
        }
        if (s.channelInput != previousChannel) historyClear();
        failCount = 0;
        noticeShown = false;
    }

    if (!connectWifi()) {
        transientFailure("Sin conexión WiFi", "No se pudo conectar a «" + WiFi.SSID() + "».", RETRY_MIN,
                         s.refreshMin);
    }
    syncClock();

    ChannelStats st;
    String error;
    switch (youtubeFetch(s, st, error)) {
    case YtResult::Ok: {
        time_t now = time(nullptr);
        int32_t today = clockValid() ? localDay(now) : 0;
        if (today) historyRecord(today, st.subscribersHidden ? -1 : st.subscribers, st.views);
        drawDashboard(st, today, now, false);
        displayFlush();
        failCount = 0;
        noticeShown = false;
        sleepFor(s.refreshMin);
    }
    case YtResult::ChannelNotFound:
        show("Canal no encontrado", "No existe ningún canal «" + s.channelInput + "».",
             "Comprueba el nombre (por ejemplo @micanal) en la configuración.");
        sleepFor(0);
    case YtResult::BadKey:
        show("Clave de API no válida", error,
             "Revisa la clave y que la YouTube Data API v3 esté habilitada.");
        sleepFor(0);
    case YtResult::QuotaExceeded:
        transientFailure("Cuota de YouTube agotada", "Se reinicia cada día a las 9:00 (hora de España).",
                         QUOTA_RETRY_MIN, QUOTA_RETRY_MIN);
    default:
        transientFailure("No se pudo consultar YouTube", error, RETRY_MIN, s.refreshMin);
    }
}

void loop() {}  // nunca se llega aquí

#endif
