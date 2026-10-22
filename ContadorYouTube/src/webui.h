#pragma once
#include <Arduino.h>
#include "settings.h"
#include "youtube.h"

// Página de ajustes en la red local (versiones OLED y CYD): brillo, segundos por
// vista, atenuado nocturno, canal, intervalo y clave, sin volver al portal.

enum WebChange : uint8_t { WEB_DISPLAY = 1, WEB_CHANNEL = 2, WEB_REFRESH = 4 };

void webuiBegin(Settings *settings, const ChannelStats *stats);
void webuiStop();  // antes de abrir el portal de WiFiManager (usa el mismo puerto)
void webuiLoop();
uint8_t webuiTakeChanges();  // máscara de WebChange desde la última llamada
int webuiTakeTest();         // aviso que se ha pedido probar (AlertKind) o -1
String webuiHost();          // nombre en la red, p. ej. "contador-1a2b" (se abre como contador-1a2b.local)
