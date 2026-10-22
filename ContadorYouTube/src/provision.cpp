#include "provision.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "screen.h"
#include "version.h"

#ifndef LILYGO_T5_EPD47_S3

#define SERIAL_WINDOW_MS 1200  // lo que espera al arrancar; cada orden recibida lo alarga

void provisionApply(Settings &s, const String &ssid, const String &pass, bool havePass, const String &channel,
                    const String &apiKey, String &summary)
{
    summary = "";
    if (ssid.length()) {
        WiFi.mode(WIFI_STA);
        WiFi.persistent(true);
        WiFi.begin(ssid.c_str(), havePass && pass.length() ? pass.c_str() : nullptr);  // queda guardada
        summary += "WiFi «" + ssid + "»";
    }
    if (channel.length() && channel != s.channelInput) {
        s.channelInput = channel;
        summary += String(summary.length() ? ", " : "") + "canal " + channel;
    }
    if (apiKey.length() && apiKey != s.apiKey) {
        s.apiKey = apiKey;
        summary += String(summary.length() ? ", " : "") + "clave de API";
    }
    settingsSave(s);
    if (!summary.length()) summary = "sin cambios";
}

static String savedSsid()
{
    wifi_config_t conf;
    WiFi.mode(WIFI_STA);
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) return "";
    return String((const char *)conf.sta.ssid).substring(0, sizeof(conf.sta.ssid));
}

static void sendInfo(const Settings &s)
{
    JsonDocument d;
    d["placa"] = FIRMWARE_BOARD;
    d["version"] = FIRMWARE_VERSION;
    d["canal"] = s.channelInput;
    d["wifi"] = savedSsid();
    d["clave"] = s.effectiveApiKey().length() > 0;  // solo si hay, nunca la clave
    Serial.print("INFO ");
    serializeJson(d, Serial);
    Serial.println();
}

bool provisionSerial(Settings &s)
{
    Serial.printf("CONTADOR-YT LISTO %s %s\n", FIRMWARE_BOARD, FIRMWARE_VERSION);
    bool configured = false;
    String line;
    uint32_t until = millis() + SERIAL_WINDOW_MS;
    while ((int32_t)(millis() - until) < 0) {
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\r') continue;
            if (c != '\n') {
                if (line.length() < 1024) line += c;
                continue;
            }
            line.trim();
            until = millis() + SERIAL_WINDOW_MS;
            if (line == "INFO") {
                sendInfo(s);
            } else if (line.startsWith("CONFIG ")) {
                JsonDocument d;
                if (deserializeJson(d, line.substring(7))) {
                    Serial.println("CONFIG ERROR formato");
                } else {
                    String summary;
                    provisionApply(s, d["wifi"] | "", d["contrasena"] | "", d["contrasena"].is<const char *>(),
                                   d["canal"] | "", d["clave"] | "", summary);
                    configured = true;
                    Serial.print("CONFIG OK ");
                    Serial.println(summary);  // sin la contraseña
                    screenMessage("Configuración recibida", "Por el cable USB: " + summary + ".", "Conectando...");
                    until = millis() + 300;  // ya está: sigue arrancando
                }
            }
            line = "";
        }
        delay(5);
    }
    return configured;
}

#else

void provisionApply(Settings &, const String &, const String &, bool, const String &, const String &, String &) {}
bool provisionSerial(Settings &) { return false; }

#endif
