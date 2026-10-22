#include "settings.h"
#include <Preferences.h>

static const char *NS = "ytdash";

void settingsLoad(Settings &s)
{
    Preferences p;
    p.begin(NS, true);
    s.channelInput = p.getString("channel", "");
    s.apiKey = p.getString("apikey", "");
    s.refreshMin = p.getUShort("refresh", DEFAULT_REFRESH_MIN);
    s.resolvedFor = p.getString("resfor", "");
    s.channelId = p.getString("chid", "");
    s.uploadsPlaylist = p.getString("uploads", "");
    s.brightness = p.getUChar("bright", 70);
    s.pageSeconds = p.getUChar("pagesec", 6);
    s.nightDim = p.getBool("night", true);
    s.volume = p.getUChar("volume", 50);
    s.led = p.getBool("led", true);
    s.battery = p.getBool("battery", false);
    s.ledMode = p.getUChar("ledmode", 2);
    s.ledColor = p.getULong("ledcolor", 0xFF0033);
    s.ledLevel = p.getUChar("ledlevel", 30);
    s.ledSpeed = p.getUChar("ledspeed", 1);
    s.ledOfflineBlink = p.getBool("ledoff", true);
    if (p.getBytesLength("ledbal") == 3) p.getBytes("ledbal", s.ledBalance, 3);
    p.end();
    if (s.refreshMin < MIN_REFRESH_MIN) s.refreshMin = MIN_REFRESH_MIN;
    s.brightness = constrain(s.brightness, 5, 100);
    if (s.pageSeconds > 60) s.pageSeconds = 60;
    if (s.volume > 100) s.volume = 100;
    if (s.ledMode > 4) s.ledMode = 2;
    s.ledLevel = constrain(s.ledLevel, 5, 100);
    if (s.ledSpeed > 2) s.ledSpeed = 1;
}

void settingsSave(const Settings &s)
{
    Preferences p;
    p.begin(NS, false);
    p.putString("channel", s.channelInput);
    p.putString("apikey", s.apiKey);
    p.putUShort("refresh", s.refreshMin);
    p.putString("resfor", s.resolvedFor);
    p.putString("chid", s.channelId);
    p.putString("uploads", s.uploadsPlaylist);
    p.putUChar("bright", s.brightness);
    p.putUChar("pagesec", s.pageSeconds);
    p.putBool("night", s.nightDim);
    p.putUChar("volume", s.volume);
    p.putBool("led", s.led);
    p.putBool("battery", s.battery);
    p.putUChar("ledmode", s.ledMode);
    p.putULong("ledcolor", s.ledColor);
    p.putUChar("ledlevel", s.ledLevel);
    p.putUChar("ledspeed", s.ledSpeed);
    p.putBool("ledoff", s.ledOfflineBlink);
    p.putBytes("ledbal", s.ledBalance, 3);
    p.end();
}

void settingsClear()
{
    Preferences p;
    p.begin(NS, false);
    p.clear();
    p.end();
}
