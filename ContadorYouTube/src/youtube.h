#pragma once
#include <Arduino.h>
#include "settings.h"

struct ChannelStats {
    String title;
    int64_t subscribers = -1;  // redondeado por YouTube a 3 cifras significativas
    bool subscribersHidden = false;
    int64_t views = -1;
    int32_t videos = -1;

    bool hasVideo = false;
    String videoId;
    String videoTitle;
    time_t videoPublished = 0;
    int64_t videoViews = -1;
    int64_t videoLikes = -1;
    int64_t videoComments = -1;

    int32_t newComments = 0;  // no viene de la API: comentarios nuevos aún sin ver

    // Tampoco: el vídeo reciente que más vistas gana (lo calcula el programa
    // comparando medidas de los 10 últimos vídeos, ver trends.h)
    bool hasTrend = false;
    String trendTitle;
    int64_t trendGain = -1;    // vistas ganadas en trendHours horas
    int trendHours = 0;
    int64_t trendViews = -1;   // vistas totales de ese vídeo
    float trendTimes = 0;      // cuántas veces la media de los demás (0 si no se sabe)
    float trendEngagement = -1;
    int8_t battery = -1;      // tampoco: % de batería para la cabecera, -1 = no se enseña
};

enum class YtResult { Ok, NetworkError, BadKey, QuotaExceeded, ChannelNotFound, CommentsDisabled, Other };

struct VideoComment {
    String author;
    String text;  // en una sola línea, recortado
    time_t published = 0;
    int64_t likes = 0;
    int32_t replies = 0;
};

struct VideoSummary {
    String id;
    String title;
    time_t published = 0;
    int64_t views = -1;
    int64_t likes = -1;     // -1 si el canal los oculta
    int64_t comments = -1;  // -1 si están desactivados
};

// Consulta los datos públicos del canal: 1 unidad de cuota, más 2 si
// withVideo (último vídeo). Sin withVideo, los campos del vídeo de 'out' no
// se tocan, para conservar los de la consulta anterior.
// Puede actualizar la caché de resolución del canal en 's'.
YtResult youtubeFetch(Settings &s, ChannelStats &out, String &error, bool withVideo = true);

// Solo se piden al tocarlos en la pantalla táctil:
// comentarios más recientes de un vídeo (1 unidad de cuota)...
YtResult youtubeComments(const Settings &s, const String &videoId, VideoComment *out, int max, int &count, String &error);
// ...y los últimos vídeos subidos (hasta 50), con vistas, «me gusta» y
// comentarios (2 unidades)
YtResult youtubeRecentVideos(const Settings &s, VideoSummary *out, int max, int &count, String &error);
