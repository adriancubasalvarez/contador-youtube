#pragma once
#include <Arduino.h>
#include "youtube.h"

// Lo que el programa principal pide a la pantalla. Cada pantalla lo
// implementa en su carpeta: src/oled (SSD1306 0,96") y src/cyd (táctil 2,8").

enum Page { PAGE_SUBSCRIBERS, PAGE_VIEWS, PAGE_VIDEO, PAGE_TREND, PAGE_CHART, PAGE_COUNT };

void screenInit();
void screenPage(Page page, const ChannelStats &st, int32_t today, time_t now, bool stale);
void screenSetup(const char *apName);
void screenInfo(const String &host, const String &ip);  // dónde abrir la página de ajustes
void screenMessage(const String &title, const String &line1, const String &line2 = "");
void screenProgress(const String &title, const String &line, int percent);  // barra 0-100 (actualizaciones)
void screenBrightness(uint8_t percent, bool night);  // 5-100 %; night = brillo mínimo
// Hito alcanzado (p. ej. "100.000" y "suscriptores"): celebración a pantalla completa
void screenCelebrate(const String &value, const String &what);

// --- Pantalla táctil (la OLED no tiene: screenTouch() siempre da false) ---

// true una vez por cada toque, con su posición
bool screenTouch(int &x, int &y);

// Qué hay en la posición tocada de lo último que se dibujó. 'index' es el
// día (0-13, el 13 es hoy) en los gráficos de vistas.
// TOUCH_VIDEOS: index 1 = abrirlos ordenados por lo que más crecen
enum TouchTarget { TOUCH_NONE, TOUCH_COMMENTS, TOUCH_VIDEOS, TOUCH_DAY, TOUCH_SUBSCRIBERS, TOUCH_MORE, TOUCH_SORT };
TouchTarget screenHit(int x, int y, int &index);

// Vistas de detalle. Tocar fuera de sus zonas vuelve a la rotación.
// 'status' sustituye a la lista mientras se carga o si hay un error.
void screenComments(const ChannelStats &st, const VideoComment *c, int count, int first, time_t now, const String &status);
// Últimos vídeos, de 4 en 4 desde 'first'; byGrowth: ordenados por vistas ganadas en 24 h
void screenVideos(const VideoSummary *v, int count, int first, bool byGrowth, time_t now, const String &status);
void screenDay(int32_t today, int index);              // gráfico de vistas con un día elegido
void screenSubscriberHistory(const ChannelStats &st, int32_t today);  // suscriptores, 30 días

// --- Efectos animados (pantalla a color; en la OLED no hacen nada) ---
void screenAnimate();                       // avanza el efecto en marcha: llamarlo en cada vuelta de loop()
void screenGain(int64_t from, int64_t to);  // vista de suscriptores: la cifra sube contando y "+N" flota
// Aviso que baja desde arriba sobre las vistas principales; tocarlo abre 'target'
void screenToast(TouchTarget target, const String &title, const String &text);
