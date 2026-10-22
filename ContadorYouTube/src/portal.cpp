#include "portal.h"
#include <WiFiManager.h>

#define PORTAL_TIMEOUT_S 600
#define PORTAL_ATTEMPTS 3

static const char *PORTAL_CSS =
    "<style>body{font-family:system-ui,sans-serif}"
    "button{background:#d92b2b;border-color:#d92b2b}"
    "label{font-weight:600}</style>";

bool wifiCredentialsSaved()
{
    WiFiManager wm;
    return wm.getWiFiIsSaved();
}

bool runSetupPortal(Settings &s)
{
    const bool builtinKey = strlen(YT_API_KEY) > 0;
    String refresh(s.refreshMin);

    WiFiManagerParameter intro("<p>Elige tu red WiFi arriba y escribe tu canal de YouTube.</p>");
    WiFiManagerParameter pChannel("channel", "Tu canal de YouTube (@nombre o enlace)", s.channelInput.c_str(), 128,
                                  "placeholder='@micanal' required");
    WiFiManagerParameter pRefresh("refresh", "Actualizar cada (minutos)", refresh.c_str(), 4,
                                  "type='number' min='1' max='1440'");
    // Formato de las claves de Google: "AIza" + 35 caracteres. Así el navegador
    // avisa de una clave mal copiada antes de guardar.
    static const char *KEY_ATTRS =
        " pattern='AIza[0-9A-Za-z_\\-]{35}' title='La clave empieza por AIza y tiene 39 caracteres'"
        " autocomplete='off' autocapitalize='off' spellcheck='false'";
    String keyAttrs = String(builtinKey ? "placeholder='(vacío = usar la clave del código)'" : "required") + KEY_ATTRS;
    WiFiManagerParameter pKey("apikey", builtinKey ? "Clave de API propia (opcional)" : "Tu clave de la YouTube Data API v3",
                              s.apiKey.c_str(), 64, keyAttrs.c_str());
    WiFiManagerParameter keyHelp(
        "<p style='font-size:.85em;line-height:1.4'>¿Aún no tienes clave? Es gratis y se crea una vez:<br>"
        "1. Entra en <b>console.cloud.google.com</b> y crea un proyecto.<br>"
        "2. En <b>APIs y servicios → Biblioteca</b>, habilita <b>YouTube Data API v3</b>.<br>"
        "3. En <b>Credenciales → Crear credenciales → Clave de API</b>, restríngela a esa API.<br>"
        "4. Copia la clave (empieza por <b>AIza</b>) y pégala arriba.<br>"
        "<i>Consejo: haz esta configuración desde el ordenador donde copiaste la clave, conectándolo a "
        "la WiFi " SETUP_AP_NAME ".</i></p>");

    WiFiManager wm;
    wm.setTitle("Dashboard YouTube");
    wm.setCustomHeadElement(PORTAL_CSS);
    std::vector<const char *> menu = {"wifi"};  // una única página: WiFi + canal
    wm.setMenu(menu);
    wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
    wm.setBreakAfterConfig(true);  // vuelve aunque falle la contraseña, para reintentar
    wm.setConnectTimeout(20);
    wm.addParameter(&intro);
    wm.addParameter(&pChannel);
    wm.addParameter(&pRefresh);
    wm.addParameter(&pKey);
    if (!builtinKey) wm.addParameter(&keyHelp);

    static bool saved;
    wm.setSaveConfigCallback([]() { saved = true; });

    for (int attempt = 0; attempt < PORTAL_ATTEMPTS; attempt++) {
        saved = false;
        bool connected = wm.startConfigPortal(SETUP_AP_NAME);
        if (!saved) break;  // nadie lo configuró: se agotó el tiempo

        String channel = pChannel.getValue();
        channel.trim();
        String key = pKey.getValue();
        key.trim();
        int minutes = atoi(pRefresh.getValue());
        if (channel.length()) s.channelInput = channel;
        s.apiKey = key;
        s.refreshMin = minutes >= MIN_REFRESH_MIN ? minutes : DEFAULT_REFRESH_MIN;
        settingsSave(s);

        if (connected && s.configured()) return true;
        // contraseña incorrecta o falta el canal: el portal vuelve a abrirse
    }
    return false;
}
