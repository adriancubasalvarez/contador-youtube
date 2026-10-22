#pragma once
#include <Arduino.h>
#include <time.h>

// Días desde 1970-01-01 (algoritmo de Howard Hinnant); evita depender de timegm()
inline int32_t daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

// Número de día en hora local (para agrupar el historial por días)
inline int32_t localDay(time_t t)
{
    struct tm tm;
    localtime_r(&t, &tm);
    return daysFromCivil(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

inline bool clockValid() { return time(nullptr) > 1700000000; }
