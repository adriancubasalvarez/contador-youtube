#pragma once
#include <Arduino.h>

// Historial diario guardado en la flash. La API pública no da vistas por
// día, así que se calculan restando los totales de días consecutivos.
#define HISTORY_DAYS 30

void historyRecord(int32_t day, int64_t subscribers, int64_t views);

// Vistas ganadas en cada uno de los 'n' últimos días (el último es hoy,
// todavía parcial). -1 donde falta información.
void historyDailyViews(int32_t today, int n, int64_t *out);

// Suscriptores al cierre de cada uno de los 'n' últimos días (el último es
// hoy). -1 donde falta información.
void historySubscribers(int32_t today, int n, int64_t *out);

// Cambio de suscriptores frente al registro más antiguo dentro de
// 'maxDays' días. Devuelve los días realmente cubiertos (0 si no hay datos).
int historySubscriberDelta(int32_t today, int maxDays, int64_t current, int64_t &delta);

void historyClear();
