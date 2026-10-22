#include "trends.h"

#define SNAPSHOTS 26          // una por hora: 24 h y algo de margen
#define SNAPSHOT_GAP_S 3300   // como mucho una medida guardada cada 55 min
#define MIN_SPAN_S 3000       // para comparar hacen falta al menos ~50 min

struct Snapshot {
    time_t at;
    int64_t views;
};

struct Track {
    String id;
    Snapshot snaps[SNAPSHOTS];
    int count = 0;  // medidas guardadas (las más antiguas primero)
    time_t lastSeen = 0;
};

static Track tracks[TRENDS_VIDEOS + 4];  // algo de margen para vídeos que salen de la lista

static Track *find(const String &id)
{
    for (auto &t : tracks)
        if (t.count && t.id == id) return &t;
    return nullptr;
}

void trendsRecord(const VideoSummary *videos, int count, time_t now)
{
    if (now < 1700000000) return;  // sin hora todavía
    for (int i = 0; i < count; i++) {
        const VideoSummary &v = videos[i];
        if (!v.id.length() || v.views < 0) continue;
        Track *t = find(v.id);
        if (!t) {  // nuevo: ocupa un hueco libre o el del vídeo que hace más que no aparece
            t = &tracks[0];
            for (auto &c : tracks) {
                if (!c.count) {
                    t = &c;
                    break;
                }
                if (c.lastSeen < t->lastSeen) t = &c;
            }
            t->id = v.id;
            t->count = 0;
        }
        t->lastSeen = now;
        Snapshot s{now, v.views};
        // Las medidas quedan fijas cada hora; la última es "viva" y se va
        // actualizando hasta que pasa una hora desde la fija anterior
        if (t->count >= 2 && t->snaps[t->count - 1].at - t->snaps[t->count - 2].at < SNAPSHOT_GAP_S) {
            t->snaps[t->count - 1] = s;
            continue;
        }
        if (t->count == SNAPSHOTS) {  // llena: descarta la más antigua
            memmove(t->snaps, t->snaps + 1, sizeof(Snapshot) * (SNAPSHOTS - 1));
            t->count--;
        }
        t->snaps[t->count++] = s;
    }
}

int64_t trendsGain(const String &videoId, time_t now, int &hours)
{
    hours = 0;
    Track *t = find(videoId);
    if (!t || t->count < 2) return -1;
    const Snapshot &last = t->snaps[t->count - 1];
    // La medida más cercana a hace 24 h (o la más antigua, si no hay tantas)
    const Snapshot *base = &t->snaps[0];
    for (int i = 0; i < t->count - 1; i++) {
        if (last.at - t->snaps[i].at >= 24 * 3600) base = &t->snaps[i];
    }
    long span = last.at - base->at;
    if (span < MIN_SPAN_S || last.views < base->views) return -1;
    hours = (int)((span + 1800) / 3600);
    if (hours < 1) hours = 1;
    if (hours > 24) hours = 24;
    return last.views - base->views;
}

float trendsEngagement(const VideoSummary &v)
{
    if (v.views <= 0 || (v.likes < 0 && v.comments < 0)) return -1;
    return 100.0f * ((v.likes > 0 ? v.likes : 0) + (v.comments > 0 ? v.comments : 0)) / v.views;
}
