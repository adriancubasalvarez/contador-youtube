#pragma once
// Lo mínimo de Arduino que usan src/cyd/screen.cpp y src/format.cpp, para
// compilarlos en el PC (vista previa).
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PI
#define PI 3.14159265f
#endif
#define OUTPUT 1
#define HIGH 1
#define LOW 0

inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline uint32_t millis()
{
    using namespace std::chrono;
    return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
#define constrain(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))
inline long map(long x, long in0, long in1, long out0, long out1) { return (x - in0) * (out1 - out0) / (in1 - in0) + out0; }

#if defined(_WIN32) && !defined(_POSIX_THREAD_SAFE_FUNCTIONS)
inline struct tm *localtime_r(const time_t *t, struct tm *out)
{
    *out = *localtime(t);
    return out;
}
inline struct tm *gmtime_r(const time_t *t, struct tm *out)
{
    *out = *gmtime(t);
    return out;
}
#endif

class String {
    std::string s;

public:
    String() {}
    String(const char *c) : s(c ? c : "") {}
    String(const std::string &str) : s(str) {}
    String(char c) : s(1, c) {}
    String(int v) : s(std::to_string(v)) {}
    String(long v) : s(std::to_string(v)) {}
    String(long long v) : s(std::to_string(v)) {}
    String(unsigned v) : s(std::to_string(v)) {}
    String(unsigned long v) : s(std::to_string(v)) {}
    String(unsigned long long v) : s(std::to_string(v)) {}
    String(double v, int decimals)  // como Arduino: String(3.14159, 2) -> "3.14"
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, v);
        s = buf;
    }

    unsigned length() const { return (unsigned)s.size(); }
    const char *c_str() const { return s.c_str(); }
    char operator[](unsigned i) const { return i < s.size() ? s[i] : 0; }
    int indexOf(char c, unsigned from = 0) const
    {
        auto p = s.find(c, from);
        return p == std::string::npos ? -1 : (int)p;
    }
    String substring(unsigned a, unsigned b) const { return a >= s.size() ? String() : String(s.substr(a, b - a)); }
    void remove(unsigned i) { s.erase(std::min<size_t>(i, s.size())); }
    void remove(unsigned i, unsigned n) { s.erase(i, n); }
    void trim()
    {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
    }
    bool endsWith(const String &e) const { return s.size() >= e.s.size() && s.compare(s.size() - e.s.size(), e.s.size(), e.s) == 0; }
    void replace(const String &from, const String &to)
    {
        for (size_t p = 0; (p = s.find(from.s, p)) != std::string::npos; p += to.s.size()) s.replace(p, from.s.size(), to.s);
    }
    String &operator+=(const String &o) { return s += o.s, *this; }
    String &operator+=(const char *o) { return s += o, *this; }
    String &operator+=(char c) { return s += c, *this; }
    bool operator==(const String &o) const { return s == o.s; }
    bool operator!=(const String &o) const { return s != o.s; }
    friend String operator+(const String &a, const String &b) { return String(a.s + b.s); }
    friend String operator+(const String &a, const char *b) { return String(a.s + b); }
    friend String operator+(const char *a, const String &b) { return String(a + b.s); }
};
