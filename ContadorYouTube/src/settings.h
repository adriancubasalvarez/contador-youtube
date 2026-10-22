#pragma once
#include <Arduino.h>

// La clave de secrets.h es solo para pruebas propias. El firmware que se
// publica se compila con PUBLIC_BUILD (tools/build_firmware.py): no lleva
// ninguna y cada usuario pone la suya al configurarlo.
#if __has_include("../secrets.h") && !defined(PUBLIC_BUILD)
#include "../secrets.h"
#endif
#ifndef YT_API_KEY
#define YT_API_KEY ""
#endif

// Zona horaria POSIX (por defecto, España peninsular)
#ifndef DEVICE_TZ
#define DEVICE_TZ "CET-1CEST,M3.5.0,M10.5.0/3"
#endif

#define SETUP_AP_NAME "YT-Dashboard"
// Minutos entre consultas del canal (1 unidad de cuota cada una). Cada
// usuario tiene su propia clave (10.000 unidades al día), así que el contador
// enchufado puede consultar cada 2 minutos (~720 al día) o incluso cada
// minuto. La de tinta funciona con batería y conviene espaciarlas.
#ifndef DEFAULT_REFRESH_MIN
#ifdef LILYGO_T5_EPD47_S3
#define DEFAULT_REFRESH_MIN 30
#else
#define DEFAULT_REFRESH_MIN 2
#endif
#endif
#define MIN_REFRESH_MIN 1

struct Settings {
    String channelInput;   // lo que escribió el usuario: @canal, URL o ID
    String apiKey;         // vacío => se usa YT_API_KEY
    uint16_t refreshMin = DEFAULT_REFRESH_MIN;
    uint8_t lang = 0;              // 0 español, 1 inglés (lang.h); settingsLoad lo aplica a g_lang

    // Pantalla (se cambian desde la página de ajustes en la red local)
    uint8_t brightness = 70;   // %, 5-100
    uint8_t pageSeconds = 6;   // segundos por vista; 0 = no cambia sola (solo con BOOT)
    bool nightDim = true;      // brillo mínimo de 0:00 a 7:00 (y sin avisos)

    // Avisos (solo placas con LED y altavoz, como la CYD)
    uint8_t volume = 50;       // %, 0 = sin sonido
    bool led = true;           // destellos del LED RGB en los avisos

    // Luz de ambiente del LED (cuando no hay ningún aviso)
    uint8_t ledMode = 2;          // LedMode (alerts.h): 0 apagada, 1 fija, 2 respirando, 3 arcoíris, 4 según el canal
    uint32_t ledColor = 0xFF0033; // 0xRRGGBB (rojo YouTube)
    uint8_t ledLevel = 30;        // intensidad, 5-100 %
    uint8_t ledSpeed = 1;         // 0 lenta, 1 normal, 2 rápida
    bool ledOfflineBlink = true;  // parpadeo ámbar si no hay conexión
    // Balance de blancos del LED (0-255 por color): cada partida de LEDs luce
    // distinto; con estos valores el blanco sale blanco en la FNK0114F
    uint8_t ledBalance[3] = {192, 255, 82};
    bool battery = false;      // hay batería en el conector BAT (la placa no lo puede detectar sola)

    // Caché de la resolución del canal (evita gastar cuota en cada arranque)
    String resolvedFor;
    String channelId;
    String uploadsPlaylist;

    bool configured() const { return channelInput.length() && effectiveApiKey().length(); }
    String effectiveApiKey() const { return apiKey.length() ? apiKey : String(YT_API_KEY); }
};

void settingsLoad(Settings &s);
void settingsSave(const Settings &s);
void settingsClear();

// "es", "en", "español", "english"... -> LANG_ES / LANG_EN; -1 si no se reconoce
int langParse(String text);
