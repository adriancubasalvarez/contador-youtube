#pragma once
#include <Arduino.h>
#include "settings.h"

// Configuración sin el portal del móvil: por el cable USB (el instalador de
// Windows) o desde config.txt en la tarjeta SD.

// Guarda la WiFi (queda como si se hubiera puesto en el portal) y pone en 's'
// el canal y la clave que no vengan vacíos. 'summary' dice qué ha cambiado.
void provisionApply(Settings &s, const String &ssid, const String &pass, bool havePass, const String &channel,
                    const String &apiKey, String &summary);

// Ventana breve al arrancar en la que el instalador puede preguntar y enviar
// la configuración por el puerto serie (115200 baudios), una línea por orden:
//   INFO                    -> INFO {"placa":"cyd","version":"1.2.0",...}
//   CONFIG {"wifi":"...","contrasena":"...","canal":"@...","clave":"AIza..."}
//                           -> CONFIG OK {...}  o  CONFIG ERROR <motivo>
// Devuelve true si se recibió una configuración.
bool provisionSerial(Settings &s);
