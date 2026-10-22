#pragma once
#include <Arduino.h>

String fmtInt(int64_t n);                     // 128400 -> "128.400"
String fmtCompact(int64_t n);                 // 15320 -> "15,3 K"
String fmtAgo(time_t then, time_t now);       // "hace 3 días"
String fmtAgoShort(time_t then, time_t now);  // "3 d"
