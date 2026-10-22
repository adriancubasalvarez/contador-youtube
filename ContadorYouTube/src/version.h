#pragma once
#include "../config.h"

// Versión del firmware: súbela en cada publicación (se ve en la página de ajustes)
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.5.0"
#endif

// Placa para la que se compila. Va escrita dentro del firmware para que una
// actualización solo se acepte si es para la misma placa.
#if defined(BOARD_CYD)
#define FIRMWARE_BOARD "cyd"
#elif defined(LILYGO_T5_EPD47_S3)
#define FIRMWARE_BOARD "t5-epd47"
#else
#define FIRMWARE_BOARD "oled"
#endif

#define FIRMWARE_MARK_PREFIX "CONTADOR-YT-PLACA:"
