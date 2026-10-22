#pragma once
#include <Arduino.h>

// Avisos con el LED RGB y el altavoz de la placa. Solo la CYD los tiene; en
// las demás estas funciones no hacen nada.

enum AlertKind { ALERT_COMMENT, ALERT_SUBSCRIBER, ALERT_VIDEO, ALERT_MILESTONE };

bool alertsAvailable();  // hay LED y altavoz (para enseñar sus ajustes)
void alertsInit();

// volume: 0-100 % (0 = sin sonido); quiet: horario nocturno, ni sonido ni LED
void alertsConfigure(uint8_t volume, bool led, bool quiet);

void alertPlay(AlertKind kind);                   // sonido y destello (respeta el horario nocturno)
void alertTest(AlertKind kind, uint8_t volume);  // desde la página de ajustes: suena siempre

// Señales breves encima de la luz de ambiente: doble pulso azul con
// comentarios sin ver, parpadeo ámbar sin conexión
void alertsSetUnread(bool unread);
void alertsSetOffline(bool offline);

// Luz de ambiente: lo que hace el LED cuando no hay ningún aviso
enum LedMode : uint8_t { LED_OFF, LED_FIXED, LED_BREATHE, LED_RAINBOW, LED_CHANNEL };
// color 0xRRGGBB; level 5-100 %; speed 0 lenta, 1 normal, 2 rápida
void alertsAmbient(uint8_t mode, uint32_t color, uint8_t level, uint8_t speed, bool offlineBlink);
void alertsSetTrend(int trend);  // modo "según el canal": 1 suben las vistas hoy, 0 no, -1 sin datos

// Balance de blancos (0-255 por color); se ajusta en la página de ajustes
void alertsSetBalance(uint8_t r, uint8_t g, uint8_t b);

void alertsLoop();  // anima el LED; llamarlo a menudo desde loop()
