#include "updater.h"
#include <SD.h>
#include <Update.h>
#include "screen.h"
#include "sdcard.h"
#include "version.h"

#ifndef LILYGO_T5_EPD47_S3

// Marca que va dentro de cada firmware (y que se busca en el nuevo)
extern const char FIRMWARE_MARK[];
const char FIRMWARE_MARK[] = FIRMWARE_MARK_PREFIX FIRMWARE_BOARD "|v" FIRMWARE_VERSION;
static const char WANTED[] = FIRMWARE_MARK_PREFIX FIRMWARE_BOARD "|";  // esta placa, cualquier versión

static size_t total = 0, written = 0;
static size_t matched = 0;  // caracteres de WANTED ya encontrados seguidos
static bool found = false, active = false;

bool updaterBegin(size_t size)
{
    total = size;
    written = matched = 0;
    found = false;
    active = Update.begin(size ? size : UPDATE_SIZE_UNKNOWN, U_FLASH);
    return active;
}

// Busca WANTED en el flujo, aunque quede partido entre dos trozos
static void scan(const uint8_t *data, size_t len)
{
    const size_t n = sizeof(WANTED) - 1;
    for (size_t i = 0; i < len && !found; i++) {
        if (data[i] == (uint8_t)WANTED[matched]) {
            if (++matched == n) found = true;
        } else {
            matched = data[i] == (uint8_t)WANTED[0] ? 1 : 0;
        }
    }
}

bool updaterWrite(const uint8_t *data, size_t len)
{
    if (!active) return false;
    scan(data, len);
    if (Update.write((uint8_t *)data, len) != len) return false;
    written += len;
    return true;
}

bool updaterEnd(String &error)
{
    if (!active) {
        error = "No se pudo empezar la actualización";
        return false;
    }
    active = false;
    if (!found) {
        Update.abort();
        error = String("Este archivo no es el firmware del contador para esta placa (") + FIRMWARE_BOARD + ")";
        return false;
    }
    if (!Update.end(true)) {
        error = String("Archivo dañado o incompleto: ") + Update.errorString();
        return false;
    }
    return true;
}

void updaterAbort()
{
    if (active) Update.abort();
    active = false;
}

int updaterProgress() { return total ? (int)(written * 100 / total) : 0; }

void updaterCheckSd()
{
    static const char *PATH = "/firmware.bin";
    Serial.printf("Firmware: %s\n", FIRMWARE_MARK);  // además, así la marca no se descarta al enlazar
    if (!sdAvailable() || !SD.exists(PATH)) return;
    File f = SD.open(PATH);
    if (!f) return;
    Serial.printf("Actualización: firmware.bin de %u bytes en la tarjeta\n", (unsigned)f.size());
    screenProgress("Actualizando el contador", "No lo desenchufes.", 0);
    String error;
    bool ok = updaterBegin(f.size());
    static uint8_t buf[4096];
    int lastShown = -1;
    while (ok && f.available()) {
        int n = f.read(buf, sizeof(buf));
        if (n <= 0 || !updaterWrite(buf, n)) {
            ok = false;
            error = "Error al leer o escribir el firmware";
            break;
        }
        int pct = updaterProgress();
        if (pct / 5 != lastShown) {  // la pantalla, cada 5 %
            lastShown = pct / 5;
            screenProgress("Actualizando el contador", "No lo desenchufes.", pct);
        }
    }
    f.close();
    if (ok) ok = updaterEnd(error);
    else updaterAbort();
    // Se renombra siempre, para no repetirlo en cada arranque
    const char *renamed = ok ? "/firmware_instalado.bin" : "/firmware_rechazado.bin";
    SD.remove(renamed);
    SD.rename(PATH, renamed);
    if (ok) {
        Serial.println("Actualización: hecha, reiniciando");
        screenProgress("Actualización terminada", "Reiniciando...", 100);
        delay(1500);
        ESP.restart();
    }
    Serial.printf("Actualización: rechazada (%s)\n", error.c_str());
    screenMessage("Actualización no instalada", error + ".", "Sigue el firmware actual.");
    delay(6000);
}

#endif
