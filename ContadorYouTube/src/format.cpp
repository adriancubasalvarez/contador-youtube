#include "format.h"
#include "lang.h"

String fmtInt(int64_t n)
{
    if (n < 0) return "—";
    String digits = String((unsigned long long)n), out;
    int len = digits.length();
    for (int i = 0; i < len; i++) {
        out += digits[i];
        if ((len - i - 1) % 3 == 0 && i != len - 1) out += langEn() ? ',' : '.';
    }
    return out;
}

String fmtCompact(int64_t n)
{
    if (n < 0) return "—";
    if (n < 10000) return fmtInt(n);
    double v = n;
    const char *suffix = " K";
    if (n >= 1000000000LL) {
        v /= 1e9;
        suffix = " B";
    } else if (n >= 1000000) {
        v /= 1e6;
        suffix = " M";
    } else {
        v /= 1e3;
    }
    char buf[24];
    snprintf(buf, sizeof(buf), v >= 100 ? "%.0f" : "%.1f", v);
    String s(buf);
    if (!langEn()) s.replace(".", ",");
    if (s.endsWith(",0") || s.endsWith(".0")) s.remove(s.length() - 2);
    return s + suffix;
}

String fmtAgo(time_t then, time_t now)
{
    if (!then || now < then) return "";
    long mins = (now - then) / 60;
    if (mins < 60) return langEn() ? String(mins) + " min ago" : "hace " + String(mins) + " min";
    long hours = mins / 60;
    if (hours < 24) return langEn() ? String(hours) + " h ago" : "hace " + String(hours) + " h";
    long days = hours / 24;
    if (days == 1) return TR("ayer", "yesterday");
    if (days < 31) return langEn() ? String(days) + " days ago" : "hace " + String(days) + " días";
    long months = days / 30;
    if (months < 12) {
        return langEn() ? String(months) + (months == 1 ? " month ago" : " months ago")
                        : "hace " + String(months) + (months == 1 ? " mes" : " meses");
    }
    long years = days / 365;
    return langEn() ? String(years) + (years == 1 ? " year ago" : " years ago")
                    : "hace " + String(years) + (years == 1 ? " año" : " años");
}

String fmtAgoShort(time_t then, time_t now)
{
    if (!then || now < then) return "";
    long mins = (now - then) / 60;
    if (mins < 60) return String(mins) + " min";
    if (mins < 24 * 60) return String(mins / 60) + " h";
    long days = mins / (24 * 60);
    if (days < 31) return String(days) + " d";
    if (days < 365) return String(days / 30) + TR(" mes", " mo");
    return String(days / 365) + TR(" a", " y");
}
