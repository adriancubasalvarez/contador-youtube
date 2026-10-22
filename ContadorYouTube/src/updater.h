#pragma once
#include <Arduino.h>

// Actualización del firmware sin cable (desde la tarjeta SD o la página de
// ajustes). Se escribe en el hueco libre de la memoria y solo se activa si
// llega entera y es para esta placa: si algo falla, sigue el firmware actual.

bool updaterBegin(size_t size);                   // size: tamaño del .bin
bool updaterWrite(const uint8_t *data, size_t len);
bool updaterEnd(String &error);                    // true: listo para reiniciar
void updaterAbort();
int updaterProgress();                             // 0-100

// firmware.bin en la tarjeta SD: si existe, lo instala (con barra de progreso
// en pantalla), lo renombra a firmware_instalado.bin y reinicia. Si no es
// válido, avisa, lo renombra a firmware_rechazado.bin y sigue arrancando.
void updaterCheckSd();
