#include "youtube.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "certs.h"
#include "util.h"

static const char *API = "https://www.googleapis.com/youtube/v3/";

static String urlEncode(const String &s)
{
    static const char *hex = "0123456789ABCDEF";
    String out;
    for (size_t i = 0; i < s.length(); i++) {
        uint8_t c = s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

// "@canal", "canal", "https://youtube.com/@canal", ".../channel/UC...", "UC..."
// -> parámetro de búsqueda para channels.list
static String channelSelector(String in)
{
    in.trim();
    int p = in.indexOf("youtube.com/");
    if (p >= 0) {
        in = in.substring(p + 12);
        int end = in.indexOf('?');
        if (end >= 0) in = in.substring(0, end);
        if (in.startsWith("channel/")) {
            in = in.substring(8);
            end = in.indexOf('/');
            return "id=" + urlEncode(end >= 0 ? in.substring(0, end) : in);
        }
        if (in.startsWith("user/")) {
            in = in.substring(5);
            end = in.indexOf('/');
            return "forUsername=" + urlEncode(end >= 0 ? in.substring(0, end) : in);
        }
        if (in.startsWith("c/")) in = in.substring(2);  // URL personalizada antigua: se prueba como @
        end = in.indexOf('/');
        if (end >= 0) in = in.substring(0, end);
    }
    if (in.length() == 24 && in.startsWith("UC")) return "id=" + in;
    if (!in.startsWith("@")) in = "@" + in;
    return "forHandle=" + urlEncode(in);
}

static int64_t toInt64(JsonVariantConst v)
{
    const char *s = v.as<const char *>();  // la API devuelve los contadores como texto
    return s ? atoll(s) : -1;
}

static time_t parseIso8601(const char *s)  // "2026-10-14T16:00:05Z", siempre UTC
{
    int y, mo, d, h, mi, se;
    if (!s || sscanf(s, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &se) != 6) return 0;
    return (time_t)((int64_t)daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60 + se);
}

static YtResult classifyError(int code, JsonDocument &doc, String &error)
{
    const char *reason = doc["error"]["errors"][0]["reason"] | "";
    const char *message = doc["error"]["message"] | "";
    error = String(code) + " " + reason + ": " + message;
    if (!strcmp(reason, "quotaExceeded") || !strcmp(reason, "dailyLimitExceeded")) return YtResult::QuotaExceeded;
    if (!strcmp(reason, "commentsDisabled")) return YtResult::CommentsDisabled;
    if (code == 400 || code == 403) {
        if (strstr(message, "API key") || !strcmp(reason, "keyInvalid") || !strcmp(reason, "accessNotConfigured") ||
            !strcmp(reason, "forbidden"))
            return YtResult::BadKey;
    }
    return YtResult::Other;
}

static YtResult getJson(const String &path, const String &key, JsonDocument &doc, String &error)
{
    WiFiClientSecure client;
    client.setCACert(GOOGLE_ROOT_CA);
    HTTPClient http;
    http.setTimeout(15000);
    http.useHTTP10(true);  // respuesta sin "chunked" para poder leerla en streaming
    if (!http.begin(client, String(API) + path + "&key=" + key)) {
        error = "No se pudo iniciar la conexión";
        return YtResult::NetworkError;
    }
    int code = http.GET();
    if (code <= 0) {
        error = HTTPClient::errorToString(code);
        http.end();
        return YtResult::NetworkError;
    }
    DeserializationError jerr = deserializeJson(doc, http.getStream());
    http.end();
    if (code != 200) return classifyError(code, doc, error);
    if (jerr) {
        error = String("JSON: ") + jerr.c_str();
        return YtResult::Other;
    }
    return YtResult::Ok;
}

YtResult youtubeFetch(Settings &s, ChannelStats &out, String &error, bool withVideo)
{
    const String key = s.effectiveApiKey();
    JsonDocument doc;

    // 1) Canal (por ID en caché si ya se resolvió antes)
    String selector = (s.resolvedFor == s.channelInput && s.channelId.length())
                          ? "id=" + s.channelId
                          : channelSelector(s.channelInput);
    YtResult r = getJson("channels?part=snippet,statistics,contentDetails&" + selector +
                             "&fields=items(id,snippet/title,statistics,contentDetails/relatedPlaylists/uploads)",
                         key, doc, error);
    if (r != YtResult::Ok) return r;
    JsonObjectConst ch = doc["items"][0];
    if (ch.isNull()) {
        error = "Canal no encontrado: " + s.channelInput;
        return YtResult::ChannelNotFound;
    }
    out.title = ch["snippet"]["title"] | "";
    out.subscribersHidden = ch["statistics"]["hiddenSubscriberCount"] | false;
    out.subscribers = toInt64(ch["statistics"]["subscriberCount"]);
    out.views = toInt64(ch["statistics"]["viewCount"]);
    out.videos = (int32_t)toInt64(ch["statistics"]["videoCount"]);
    String uploads = ch["contentDetails"]["relatedPlaylists"]["uploads"] | "";
    String id = ch["id"] | "";
    if (s.resolvedFor != s.channelInput || s.channelId != id || s.uploadsPlaylist != uploads) {
        s.resolvedFor = s.channelInput;
        s.channelId = id;
        s.uploadsPlaylist = uploads;
        settingsSave(s);
    }
    if (!withVideo) return YtResult::Ok;
    if (!uploads.length() || out.videos == 0) {
        out.hasVideo = false;
        return YtResult::Ok;
    }

    // 2) Último vídeo subido. Si falla, se muestra el resto igualmente.
    doc.clear();
    String ignored;
    if (getJson("playlistItems?part=contentDetails&maxResults=1&playlistId=" + uploads +
                    "&fields=items/contentDetails/videoId",
                key, doc, ignored) != YtResult::Ok)
        return YtResult::Ok;
    String videoId = doc["items"][0]["contentDetails"]["videoId"] | "";
    if (!videoId.length()) return YtResult::Ok;

    doc.clear();
    if (getJson("videos?part=snippet,statistics&id=" + videoId +
                    "&fields=items(snippet(title,publishedAt),statistics(viewCount,likeCount,commentCount))",
                key, doc, ignored) != YtResult::Ok)
        return YtResult::Ok;
    JsonObjectConst v = doc["items"][0];
    if (v.isNull()) return YtResult::Ok;
    out.hasVideo = true;
    out.videoId = videoId;
    out.videoTitle = v["snippet"]["title"] | "";
    out.videoPublished = parseIso8601(v["snippet"]["publishedAt"]);
    out.videoViews = toInt64(v["statistics"]["viewCount"]);
    out.videoLikes = toInt64(v["statistics"]["likeCount"]);  // -1 si los oculta
    out.videoComments = toInt64(v["statistics"]["commentCount"]);
    return YtResult::Ok;
}

// Texto en una sola línea y de longitud limitada (la pantalla muestra 3 líneas)
static String oneLine(const char *s, size_t maxBytes)
{
    String out;
    for (; s && *s && out.length() < maxBytes; s++) out += (*s == '\n' || *s == '\r' || *s == '\t') ? ' ' : *s;
    while (out.indexOf("  ") >= 0) out.replace("  ", " ");
    out.trim();
    return out;
}

YtResult youtubeComments(const Settings &s, const String &videoId, VideoComment *out, int max, int &count, String &error)
{
    count = 0;
    JsonDocument doc;
    YtResult r = getJson("commentThreads?part=snippet&order=time&textFormat=plainText&maxResults=" + String(max) +
                             "&videoId=" + videoId +
                             "&fields=items/snippet(totalReplyCount,topLevelComment/snippet(authorDisplayName,"
                             "textOriginal,publishedAt,likeCount))",
                         s.effectiveApiKey(), doc, error);
    if (r != YtResult::Ok) return r;
    for (JsonObjectConst item : doc["items"].as<JsonArrayConst>()) {
        if (count >= max) break;
        JsonObjectConst c = item["snippet"]["topLevelComment"]["snippet"];
        VideoComment &vc = out[count++];
        vc.author = c["authorDisplayName"] | "";
        vc.text = oneLine(c["textOriginal"], 400);
        vc.published = parseIso8601(c["publishedAt"]);
        vc.likes = c["likeCount"] | 0;
        vc.replies = item["snippet"]["totalReplyCount"] | 0;
    }
    return YtResult::Ok;
}

YtResult youtubeRecentVideos(const Settings &s, VideoSummary *out, int max, int &count, String &error)
{
    count = 0;
    if (!s.uploadsPlaylist.length()) {
        error = "Canal sin resolver";
        return YtResult::Other;
    }
    const String key = s.effectiveApiKey();
    JsonDocument doc;
    YtResult r = getJson("playlistItems?part=contentDetails&maxResults=" + String(max) + "&playlistId=" +
                             s.uploadsPlaylist + "&fields=items/contentDetails/videoId",
                         key, doc, error);
    if (r != YtResult::Ok) return r;
    String ids;
    for (JsonObjectConst item : doc["items"].as<JsonArrayConst>()) {
        const char *id = item["contentDetails"]["videoId"];
        if (id) ids += (ids.length() ? "%2C" : "") + String(id);
    }
    if (!ids.length()) return YtResult::Ok;

    doc.clear();
    r = getJson("videos?part=snippet,statistics&id=" + ids +
                    "&fields=items(id,snippet(title,publishedAt),statistics(viewCount,likeCount,commentCount))",
                key, doc, error);
    if (r != YtResult::Ok) return r;
    for (JsonObjectConst v : doc["items"].as<JsonArrayConst>()) {  // en el mismo orden pedido
        if (count >= max) break;
        VideoSummary &vs = out[count++];
        vs.id = v["id"] | "";
        vs.title = v["snippet"]["title"] | "";
        vs.published = parseIso8601(v["snippet"]["publishedAt"]);
        vs.views = toInt64(v["statistics"]["viewCount"]);
        vs.likes = toInt64(v["statistics"]["likeCount"]);
        vs.comments = toInt64(v["statistics"]["commentCount"]);
    }
    return YtResult::Ok;
}
