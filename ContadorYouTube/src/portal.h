#pragma once
#include "settings.h"

// Crea la red WiFi abierta SETUP_AP_NAME con portal cautivo. Devuelve true
// cuando el usuario ha guardado su WiFi y su canal y la conexión funciona.
bool runSetupPortal(Settings &s);

bool wifiCredentialsSaved();
