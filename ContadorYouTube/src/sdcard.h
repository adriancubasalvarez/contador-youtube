#pragma once
#include <Arduino.h>
#include "settings.h"
#include "youtube.h"

// Tarjeta microSD de la CYD (en las demás placas no hay: todo devuelve false
// o vacío). Si no hay tarjeta, el contador funciona igual.

bool sdBegin();      // monta la tarjeta; false si no hay
bool sdAvailable();
String sdDescribe(); // para la página de ajustes, p. ej. "7,4 GB · historial 120 KB"

// config.txt: configuración sin móvil (WiFi, contraseña, canal y clave). Si
// existe, guarda la WiFi, pone en 's' el canal y la clave y lo renombra a
// config_aplicada.txt con la contraseña tapada. Devuelve true si se aplicó;
// 'summary' dice qué se cambió.
bool sdApplyConfig(Settings &s, String &summary);

// historial.csv: una línea por consulta (se abre con Excel o LibreOffice)
void sdLog(time_t now, const ChannelStats &st);
bool sdHasHistory();
const char *sdHistoryPath();

// Sonido propio para un aviso (AlertKind): ruta del WAV en la tarjeta o ""
String sdSoundPath(int alertKind);
