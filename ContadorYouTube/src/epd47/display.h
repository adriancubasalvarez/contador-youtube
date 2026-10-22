#pragma once
#include <Arduino.h>
#include "../youtube.h"

bool displayInit();  // reserva el framebuffer en PSRAM
void displayFlush(); // envía el framebuffer a la pantalla y la apaga

float batteryVoltage();

void drawDashboard(const ChannelStats &st, int32_t today, time_t now, bool stale);
void drawSetupScreen(const char *apName);
void drawMessage(const String &title, const String &line1, const String &line2 = "");
