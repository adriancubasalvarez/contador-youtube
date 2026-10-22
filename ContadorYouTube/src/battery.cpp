#include "battery.h"
#include "../config.h"

#if defined(BOARD_CYD) && !defined(LILYGO_T5_EPD47_S3)

// Freenove FNK0114F (esquema oficial): BAT+ por un divisor 100k/100k al GPIO34
#define BAT_ADC 34
// Sin batería, el cargador TP4054 deja ~4,17 V en ese pin (medido): no se
// distingue de una batería llena, así que es el usuario quien dice si hay
// batería. Fuera de estos límites la medida no es de una LiPo.
#define BATTERY_MIN_MV 3000
#define BATTERY_MAX_MV 4350

int batteryMillivolts()
{
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(BAT_ADC);  // media: el ADC es ruidoso
    return (int)(sum / 16) * 2;
}

// Curva de descarga típica de una LiPo de una celda (mV -> %)
static const int16_t CURVE[][2] = {{4180, 100}, {4100, 92}, {4000, 81}, {3900, 69}, {3800, 54},
                                   {3750, 44}, {3700, 32}, {3650, 20}, {3600, 12}, {3500, 5}, {3300, 0}};

int batteryPercent()
{
    int mv = batteryMillivolts();
    if (mv < BATTERY_MIN_MV || mv > BATTERY_MAX_MV) return -1;  // sin batería (fuera de lo posible)
    for (size_t i = 1; i < sizeof(CURVE) / sizeof(CURVE[0]); i++) {
        if (mv >= CURVE[i][0]) {
            int v0 = CURVE[i][0], v1 = CURVE[i - 1][0], p0 = CURVE[i][1], p1 = CURVE[i - 1][1];
            return constrain(p0 + (mv - v0) * (p1 - p0) / (v1 - v0), 0, 100);
        }
    }
    return 0;
}

#else

int batteryMillivolts() { return -1; }
int batteryPercent() { return -1; }

#endif
