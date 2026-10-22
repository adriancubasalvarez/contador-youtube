#pragma once
#include <Arduino.h>
#include "youtube.h"

// Cuánto crece cada uno de los últimos vídeos. La API pública no da las vistas
// por día, así que se guardan en memoria medidas de sus vistas (una por hora,
// 26 horas) y se compara la de ahora con la de hace 24 h. Tras encender, la
// comparación es con la medida más antigua que haya (menos horas).

#define TRENDS_VIDEOS 10

void trendsRecord(const VideoSummary *videos, int count, time_t now);

// Vistas ganadas por ese vídeo en las últimas 24 h, o en las 'hours' que haya
// medidas (al menos 1). -1 si todavía no hay dos medidas.
int64_t trendsGain(const String &videoId, time_t now, int &hours);

// (me gusta + comentarios) / vistas, en %; -1 si no se puede calcular
float trendsEngagement(const VideoSummary &v);
