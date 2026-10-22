#pragma once
#include <stdint.h>

// Idioma de la pantalla, la página de ajustes y los avisos. Se elige en el
// instalador, en la página de ajustes, en el portal de configuración o con
// idioma=en en config.txt (Settings::lang).
enum : uint8_t { LANG_ES = 0, LANG_EN = 1 };
inline uint8_t g_lang = LANG_ES;

inline bool langEn() { return g_lang == LANG_EN; }

// TR("texto", "text"): el de la lengua elegida
#define TR(es, en) (g_lang == LANG_EN ? (en) : (es))
