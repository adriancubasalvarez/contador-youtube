#include "portal.h"
#include <WiFiManager.h>
#include "lang.h"

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

    // Idioma del contador: un desplegable que rellena el campo oculto "lang"
    String langHtml = String("<label for='langsel'>Idioma · Language</label><select id='langsel' "
                             "onchange=\"document.getElementById('lang').value=this.value\">"
                             "<option value='es'") +
                      (s.lang == LANG_ES ? " selected" : "") + ">Español</option><option value='en'" +
                      (s.lang == LANG_EN ? " selected" : "") + ">English</option></select>";
    WiFiManagerParameter pLangSelect(langHtml.c_str());
    WiFiManagerParameter pLang("lang", "", s.lang == LANG_EN ? "en" : "es", 3, "type='hidden'");
    WiFiManagerParameter intro(TR("<p>Elige tu red WiFi arriba y escribe tu canal de YouTube.</p>",
                                  "<p>Pick your WiFi network above and type your YouTube channel.</p>"));
    WiFiManagerParameter pChannel("channel", TR("Tu canal de YouTube (@nombre o enlace)", "Your YouTube channel (@handle or link)"),
                                  s.channelInput.c_str(), 128, "placeholder='@mychannel' required");
    WiFiManagerParameter pRefresh("refresh", TR("Actualizar cada (minutos)", "Refresh every (minutes)"), refresh.c_str(), 4,
                                  "type='number' min='1' max='1440'");
    // Formato de las claves de Google: "AIza" + 35 caracteres. Así el navegador
    // avisa de una clave mal copiada antes de guardar.
    String keyAttrs =
        String(builtinKey ? TR("placeholder='(vacío = usar la clave del código)'", "placeholder='(empty = use the built-in key)'")
                          : "required") +
        " pattern='AIza[0-9A-Za-z_\\-]{35}' title='" +
        TR("La clave empieza por AIza y tiene 39 caracteres", "The key starts with AIza and is 39 characters long") +
        "' autocomplete='off' autocapitalize='off' spellcheck='false'";
    WiFiManagerParameter pKey("apikey",
                              builtinKey ? TR("Clave de API propia (opcional)", "Your own API key (optional)")
                                         : TR("Tu clave de la YouTube Data API v3", "Your YouTube Data API v3 key"),
                              s.apiKey.c_str(), 64, keyAttrs.c_str());
    WiFiManagerParameter keyHelp(TR(
        "<p style='font-size:.85em;line-height:1.4'>¿Aún no tienes clave? Es gratis y se crea una vez:<br>"
        "1. Entra en <b>console.cloud.google.com</b> y crea un proyecto.<br>"
        "2. En <b>APIs y servicios → Biblioteca</b>, habilita <b>YouTube Data API v3</b>.<br>"
        "3. En <b>Credenciales → Crear credenciales → Clave de API</b>, restríngela a esa API.<br>"
        "4. Copia la clave (empieza por <b>AIza</b>) y pégala arriba.<br>"
        "<i>Consejo: haz esta configuración desde el ordenador donde copiaste la clave, conectándolo a "
        "la WiFi " SETUP_AP_NAME ".</i></p>",
        "<p style='font-size:.85em;line-height:1.4'>No key yet? It is free and you only create it once:<br>"
        "1. Go to <b>console.cloud.google.com</b> and create a project.<br>"
        "2. In <b>APIs &amp; Services → Library</b>, enable <b>YouTube Data API v3</b>.<br>"
        "3. In <b>Credentials → Create credentials → API key</b>, restrict it to that API.<br>"
        "4. Copy the key (it starts with <b>AIza</b>) and paste it above.<br>"
        "<i>Tip: do this setup from the computer where you copied the key, connecting it to "
        "the " SETUP_AP_NAME " WiFi.</i></p>"));

    WiFiManager wm;
    wm.setTitle(TR("Contador YouTube", "YouTube Counter"));
    wm.setCustomHeadElement(PORTAL_CSS);
    std::vector<const char *> menu = {"wifi"};  // una única página: WiFi + canal
    wm.setMenu(menu);
    wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
    wm.setBreakAfterConfig(true);  // vuelve aunque falle la contraseña, para reintentar
    wm.setConnectTimeout(20);
    wm.addParameter(&pLangSelect);
    wm.addParameter(&pLang);
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
        int lang = langParse(pLang.getValue());
        if (lang >= 0) s.lang = lang;
        if (channel.length()) s.channelInput = channel;
        s.apiKey = key;
        s.refreshMin = minutes >= MIN_REFRESH_MIN ? minutes : DEFAULT_REFRESH_MIN;
        settingsSave(s);

        if (connected && s.configured()) return true;
        // contraseña incorrecta o falta el canal: el portal vuelve a abrirse
    }
    return false;
}
