#pragma once
#include <Arduino.h>

// Batería LiPo de 3,7 V en el conector BAT de la CYD (la carga el chip
// TP4054 cuando hay USB). En las placas sin ese conector: -1.

int batteryMillivolts();  // tensión de la batería, o -1 si no hay medida
int batteryPercent();     // 0-100, o -1 si no hay batería conectada
