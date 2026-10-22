#include "history.h"
#include <Preferences.h>

struct DaySnap {
    int32_t day;  // 0 = vacío
    int64_t subscribers;
    int64_t views;
};

static DaySnap snaps[HISTORY_DAYS];
static bool loaded = false;
static const char *NS = "ythist";

static void load()
{
    if (loaded) return;
    memset(snaps, 0, sizeof(snaps));
    Preferences p;
    if (p.begin(NS, true)) {
        p.getBytes("snaps", snaps, sizeof(snaps));
        p.end();
    }
    loaded = true;
}

static const DaySnap *find(int32_t day)
{
    for (auto &s : snaps)
        if (s.day == day) return &s;
    return nullptr;
}

void historyRecord(int32_t day, int64_t subscribers, int64_t views)
{
    load();
    DaySnap *slot = nullptr;
    for (auto &s : snaps)
        if (s.day == day) slot = &s;
    if (!slot) {  // reutiliza el hueco más antiguo
        slot = &snaps[0];
        for (auto &s : snaps)
            if (s.day < slot->day) slot = &s;
    }
    if (slot->day == day && slot->subscribers == subscribers && slot->views == views) return;
    *slot = {day, subscribers, views};  // el último valor del día queda como cierre
    Preferences p;
    p.begin(NS, false);
    p.putBytes("snaps", snaps, sizeof(snaps));
    p.end();
}

void historyDailyViews(int32_t today, int n, int64_t *out)
{
    load();
    for (int i = 0; i < n; i++) {
        int32_t day = today - (n - 1 - i);
        const DaySnap *cur = find(day), *prev = find(day - 1);
        out[i] = (cur && prev && cur->views >= 0 && prev->views >= 0 && cur->views >= prev->views)
                     ? cur->views - prev->views
                     : -1;
    }
}

void historySubscribers(int32_t today, int n, int64_t *out)
{
    load();
    for (int i = 0; i < n; i++) {
        const DaySnap *s = find(today - (n - 1 - i));
        out[i] = s ? s->subscribers : -1;
    }
}

int historySubscriberDelta(int32_t today, int maxDays, int64_t current, int64_t &delta)
{
    load();
    if (current < 0) return 0;
    for (int back = maxDays; back >= 1; back--) {
        const DaySnap *s = find(today - back);
        if (s && s->subscribers >= 0) {
            delta = current - s->subscribers;
            return back;
        }
    }
    return 0;
}

void historyClear()
{
    Preferences p;
    p.begin(NS, false);
    p.clear();
    p.end();
    memset(snaps, 0, sizeof(snaps));
    loaded = true;
}
