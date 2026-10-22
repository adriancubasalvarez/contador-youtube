// Página de ajustes en la red local (versiones OLED y CYD; la de tinta no la usa).
#ifndef LILYGO_T5_EPD47_S3

#include "webui.h"
#include <ESPmDNS.h>
#include <SD.h>
#include <WebServer.h>
#include <WiFi.h>
#include "alerts.h"
#include "format.h"
#include "sdcard.h"
#include "screen.h"
#include "updater.h"
#include "version.h"

static WebServer server(80);
static Settings *cfg = nullptr;
static const ChannelStats *data = nullptr;
static bool running = false;
static uint8_t changes = 0;
static int testKind = -1;
static uint32_t restartAt = 0;  // tras actualizar: reiniciar en cuanto se haya enviado la respuesta
static String updateError;
static String host;

static const uint8_t PAGE_SECONDS_OPTIONS[] = {0, 3, 4, 5, 6, 8, 10, 15, 20, 30, 60};
static const char *const LED_MODES[] = {"Apagada", "Fija", "Respirando", "Arcoíris lento",
                                        "Según el canal (verde si hoy suben las vistas)"};
static const char *const LED_SPEEDS[] = {"Lenta", "Normal", "Rápida"};
// Colores predefinidos: nombre y 0xRRGGBB
static const struct {
    const char *name;
    uint32_t rgb;
} LED_PRESETS[] = {{"Rojo YouTube", 0xFF0033}, {"Blanco cálido", 0xFFB060}, {"Azul", 0x2060FF}, {"Verde", 0x20FF50},
                   {"Morado", 0x9030FF}, {"Naranja", 0xFF6000}, {"Rosa", 0xFF3090}, {"Cian", 0x00D0FF}};

static String hexColor(uint32_t c)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "#%06lX", (unsigned long)(c & 0xFFFFFF));
    return buf;
}

static String esc(const String &s)
{
    String out;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '&') out += "&amp;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else out += c;
    }
    return out;
}

static const char *STYLE =
    "<style>"
    ":root{--bg:#f4f4f2;--card:#fff;--text:#1b1b1b;--muted:#6b6b6b;--line:#e2e2de;--accent:#d92b2b}"
    "@media(prefers-color-scheme:dark){:root{--bg:#161616;--card:#222;--text:#ececec;--muted:#a0a0a0;"
    "--line:#353535;--accent:#ff5a5a}}"
    "*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);"
    "font:16px/1.5 system-ui,-apple-system,'Segoe UI',Roboto,sans-serif}"
    "main{max-width:520px;margin:0 auto;padding:16px}"
    "h1{font-size:1.2rem;margin:8px 0 14px}h1 b{color:var(--accent)}"
    ".card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:14px}"
    ".stat{font-size:2rem;font-weight:700;line-height:1.1}.muted{color:var(--muted);font-size:.88rem}"
    "label{display:block;font-weight:600;font-size:.92rem;margin:12px 0 4px}"
    "input[type=text],input[type=number],select{font:inherit;width:100%;padding:8px 10px;border:1px solid var(--line);"
    "border-radius:8px;background:var(--bg);color:var(--text)}input[type=range]{width:100%}"
    ".check{display:flex;gap:8px;align-items:center;font-weight:400}"
    "button{font:inherit;font-weight:600;margin-top:16px;width:100%;padding:10px;border:0;border-radius:8px;"
    "background:var(--accent);color:#fff}"
    ".sw{display:flex;flex-wrap:wrap;gap:6px;align-items:center}.sw button{width:30px;height:30px;margin:0;"
    "padding:0;border-radius:50%;border:2px solid var(--line)}.sw input{width:44px;height:34px;padding:2px}"
    ".tests{display:grid;grid-template-columns:1fr 1fr;gap:8px}.tests button{margin-top:0}"
    ".ok{background:#e7f5ea;color:#1f6b32;padding:10px 12px;border-radius:8px;margin-bottom:14px}"
    "</style>";

static String page(const String &notice)
{
    String h;
    h.reserve(5000);
    h += F("<!doctype html><html lang='es'><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'><title>Contador YouTube</title>");
    h += STYLE;
    h += F("</head><body><main><h1><b>&#9654;</b> Contador YouTube &middot; Ajustes</h1>");
    if (notice.length()) h += "<div class='ok'>" + notice + "</div>";

    h += F("<div class='card'>");
    if (data && data->title.length()) {
        h += "<div class='muted'>" + esc(data->title) + "</div>";
        h += "<div class='stat'>" + fmtInt(data->subscribers) + "</div><div class='muted'>suscriptores</div>";
    } else {
        h += F("<div class='muted'>Todavía no hay datos del canal.</div>");
    }
    h += F("</div><form method='post' action='/guardar'><div class='card'>");

    h += F("<label for='b'>Brillo: <span id='bv'>");
    h += String(cfg->brightness);
    h += F("</span> %</label><input type='range' id='b' name='brillo' min='5' max='100' step='5' value='");
    h += String(cfg->brightness);
    h += F("' oninput=\"bv.textContent=this.value;fetch('/probar?brillo='+this.value)\">");

    h += F("<label for='p'>Cambiar de vista cada</label><select id='p' name='vista'>");
    for (uint8_t v : PAGE_SECONDS_OPTIONS) {
        h += "<option value='" + String(v) + "'" + (v == cfg->pageSeconds ? " selected" : "") + ">";
        h += v ? String(v) + " segundos" : String("No cambiar (solo con el botón BOOT)");
        h += "</option>";
    }
    h += F("</select>");

    h += F("<label class='check'><input type='checkbox' name='noche' value='1'");
    if (cfg->nightDim) h += F(" checked");
    h += alertsAvailable() ? F("> De noche (0:00 a 7:00), brillo mínimo y sin avisos</label></div>")
                           : F("> Bajar el brillo de noche (0:00 a 7:00)</label></div>");

    if (alertsAvailable()) {
        h += F("<div class='card'><label for='v'>Volumen de los avisos: <span id='vv'>");
        h += cfg->volume ? String(cfg->volume) + " %" : String("sin sonido");
        h += F("</span></label><input type='range' id='v' name='volumen' min='0' max='100' step='10' value='");
        h += String(cfg->volume);
        h += F("' oninput=\"vv.textContent=this.value>0?this.value+' %':'sin sonido'\" "
               "onchange=\"fetch('/probar?volumen='+this.value)\">"
               "<label class='check'><input type='checkbox' name='led' value='1'");
        if (cfg->led) h += F(" checked");
        h += F("> Destellos del LED en los avisos</label>"
               "<label class='check'><input type='checkbox' name='bateria' value='1'");
        if (cfg->battery) h += F(" checked");
        h += F("> Tengo una batería conectada (enseña su nivel)</label>"
               "<div class='muted'>Avisa de comentarios nuevos en tu último vídeo, de nuevos suscriptores, "
               "de vídeos nuevos y de los hitos (1.000, 10.000, 100.000…).</div>"
               "<label>Probar un aviso (con los datos de tu canal)</label><div class='tests'>"
               "<button type='button' onclick=\"fetch('/probar?aviso=3')\">Hito</button>"
               "<button type='button' onclick=\"fetch('/probar?aviso=1')\">Suscriptor</button>"
               "<button type='button' onclick=\"fetch('/probar?aviso=0')\">Comentario</button>"
               "<button type='button' onclick=\"fetch('/probar?aviso=2')\">Vídeo</button></div></div>");
    }
    if (alertsAvailable()) {
        // Luz de ambiente: cada cambio se ve al momento en el LED (se guarda con «Guardar»)
        h += F("<div class='card'><label for='lm'>Luz de ambiente (LED)</label><select id='lm' name='luz' onchange='lp()'>");
        for (int i = 0; i < 5; i++) {
            h += "<option value='" + String(i) + "'" + (i == cfg->ledMode ? " selected" : "") + ">" + LED_MODES[i] +
                 "</option>";
        }
        h += F("</select><label for='lc'>Color</label><div class='sw'>");
        for (auto &p : LED_PRESETS) {
            String hc = hexColor(p.rgb);
            h += "<button type='button' title='" + String(p.name) + "' style='background:" + hc +
                 "' onclick=\"lc.value='" + hc + "';lp()\"></button>";
        }
        h += F("<input type='color' id='lc' name='color' oninput='lp()' value='");
        h += hexColor(cfg->ledColor);
        h += F("'></div><label for='ll'>Intensidad: <span id='lv'>");
        h += String(cfg->ledLevel);
        h += F("</span> %</label><input type='range' id='ll' name='nivel' min='5' max='100' step='5' value='");
        h += String(cfg->ledLevel);
        h += F("' oninput='lv.textContent=this.value;lp()'><label for='ls'>Velocidad</label><select id='ls' name='vel' "
               "onchange='lp()'>");
        for (int i = 0; i < 3; i++) {
            h += "<option value='" + String(i) + "'" + (i == cfg->ledSpeed ? " selected" : "") + ">" + LED_SPEEDS[i] +
                 "</option>";
        }
        h += F("</select><label class='check'><input type='checkbox' name='parpadeo' value='1'");
        if (cfg->ledOfflineBlink) h += F(" checked");
        h += F("> Parpadeo ámbar si se queda sin conexión</label>"
               "<div class='muted'>Los avisos (comentarios, suscriptores, hitos) se ven encima y luego vuelve esta luz. "
               "Con comentarios sin ver, hace un doble pulso azul.</div>"
               "<details><summary class='muted'>Ajustar los colores del LED (balance de blancos)</summary>"
               "<div class='muted'>Mientras mueves las barras, el LED se pone blanco: déjalo lo más blanco posible, "
               "sin tinte azul ni rosa. Así los demás colores salen como en la pantalla.</div>");
        static const char *const NAMES[] = {"Rojo", "Verde", "Azul"};
        for (int i = 0; i < 3; i++) {
            String id = "b" + String(i);
            h += "<label for='" + id + "'>" + NAMES[i] + ": <span id='" + id + "v'>" + String(cfg->ledBalance[i]) +
                 "</span></label><input type='range' id='" + id + "' name='bal" + String(i) +
                 "' min='0' max='255' step='5' value='" + String(cfg->ledBalance[i]) + "' oninput='bp()'>";
        }
        h += F("<button type='button' onclick=\"b0.value=192;b1.value=255;b2.value=82;bp()\">Valores de fábrica</button>"
               "</details></div>"
               "<script>function lp(){fetch('/probar?luz='+lm.value+'&color='+lc.value.substring(1)+'&nivel='+ll.value+"
               "'&vel='+ls.value)}"
               "function bp(){b0v.textContent=b0.value;b1v.textContent=b1.value;b2v.textContent=b2.value;"
               "fetch('/probar?balance='+b0.value+','+b1.value+','+b2.value+'&luz=1&color=FFFFFF&nivel=100&vel=1')}"
               "</script>");
    }
    if (sdAvailable()) {
        h += F("<div class='card'><label>Tarjeta microSD</label><div class='muted'>");
        h += esc(sdDescribe());
        h += F("</div>");
        if (sdHasHistory())
            h += F("<p><a href='/historial.csv' download>Descargar el historial (CSV para Excel)</a></p>");
        h += F("<div class='muted'>Sonidos propios: archivos WAV en la carpeta <b>sonidos</b> llamados "
               "hito.wav, suscriptor.wav, comentario.wav o video.wav. Para configurarlo sin móvil: "
               "un archivo <b>config.txt</b> (ver el manual).</div></div>");
    }
    h += F("<div class='card'>");

    h += F("<label for='c'>Canal de YouTube</label><input type='text' id='c' name='canal' required value='");
    h += esc(cfg->channelInput);
    h += F("' placeholder='@micanal'><label for='r'>Consultar YouTube cada (minutos)</label>"
           "<input type='number' id='r' name='intervalo' min='1' max='1440' value='");
    h += String(cfg->refreshMin);
    h += F("'><label for='k'>Clave de API nueva (opcional)</label><input type='text' id='k' name='clave' "
           "autocomplete='off' autocapitalize='off' spellcheck='false' "
           "pattern='AIza[0-9A-Za-z_\\-]{35}' title='La clave empieza por AIza y tiene 39 caracteres' "
           "placeholder='Déjalo vacío para mantener la actual'>");
    String key = cfg->effectiveApiKey();
    h += "<div class='muted'>Clave actual: " +
         (key.length() > 8 ? esc(key.substring(0, 4)) + "…" + esc(key.substring(key.length() - 4)) : String("ninguna")) +
         (cfg->apiKey.length() ? "" : (key.length() ? " (grabada en el código)" : "")) + "</div>";

    h += F("<button type='submit'>Guardar</button></div></form>"
           "<p class='muted'>Para cambiar de red WiFi, mantén pulsado el botón BOOT 3 segundos.</p>"
           "<p class='muted'>Firmware " FIRMWARE_VERSION " (" FIRMWARE_BOARD ") &middot; "
           "<a href='/actualizar'>Actualizar firmware</a></p>"
           "</main></body></html>");
    return h;
}

static void sendPage(const String &notice = "")
{
    server.send(200, "text/html; charset=utf-8", page(notice));
}

static void handleSave()
{
    String channel = server.arg("canal");
    channel.trim();
    if (channel.length() && channel != cfg->channelInput) {
        cfg->channelInput = channel;
        changes |= WEB_CHANNEL;
    }
    int bright = constrain(server.arg("brillo").toInt(), 5, 100);
    int pageSec = constrain(server.arg("vista").toInt(), 0, 60);
    bool night = server.hasArg("noche");
    if (bright != cfg->brightness || pageSec != cfg->pageSeconds || night != cfg->nightDim) {
        cfg->brightness = bright;
        cfg->pageSeconds = pageSec;
        cfg->nightDim = night;
        changes |= WEB_DISPLAY;
    }
    if (alertsAvailable()) {
        int volume = constrain(server.arg("volumen").toInt(), 0, 100);
        bool led = server.hasArg("led"), battery = server.hasArg("bateria"), blink = server.hasArg("parpadeo");
        uint8_t mode = constrain(server.arg("luz").toInt(), 0, 4), level = constrain(server.arg("nivel").toInt(), 5, 100),
                speed = constrain(server.arg("vel").toInt(), 0, 2);
        uint32_t color = strtoul(server.arg("color").substring(server.arg("color").startsWith("#") ? 1 : 0).c_str(),
                                 nullptr, 16);
        if (volume != cfg->volume || led != cfg->led || battery != cfg->battery || mode != cfg->ledMode ||
            color != cfg->ledColor || level != cfg->ledLevel || speed != cfg->ledSpeed || blink != cfg->ledOfflineBlink) {
            cfg->volume = volume;
            cfg->led = led;
            cfg->battery = battery;
            cfg->ledMode = mode;
            cfg->ledColor = color;
            cfg->ledLevel = level;
            cfg->ledSpeed = speed;
            cfg->ledOfflineBlink = blink;
            changes |= WEB_DISPLAY;
        }
        for (int i = 0; i < 3; i++) {
            if (!server.hasArg("bal" + String(i))) continue;
            uint8_t v = constrain(server.arg("bal" + String(i)).toInt(), 0, 255);
            if (v != cfg->ledBalance[i]) {
                cfg->ledBalance[i] = v;
                changes |= WEB_DISPLAY;
            }
        }
    }
    int refresh = server.arg("intervalo").toInt();
    if (refresh >= MIN_REFRESH_MIN && refresh <= 1440 && refresh != cfg->refreshMin) {
        cfg->refreshMin = refresh;
        changes |= WEB_REFRESH;
    }
    String key = server.arg("clave");
    key.trim();
    if (key.length() && key != cfg->apiKey) {
        cfg->apiKey = key;
        changes |= WEB_CHANNEL;  // vuelve a consultar con la clave nueva
    }
    settingsSave(*cfg);
    sendPage("Guardado. El contador ya usa los ajustes nuevos.");
}

// Vista previa mientras se mueven los controles (no se guarda): brillo,
// volumen o un aviso de prueba
static void handlePreview()
{
    server.send(204);  // antes de sonar: el sonido bloquea un momento
    if (server.hasArg("brillo")) {
        cfg->brightness = constrain(server.arg("brillo").toInt(), 5, 100);
        changes |= WEB_DISPLAY;
    }
    if (server.hasArg("balance")) {  // calibración del LED: "r,g,b"
        int r = 255, g = 255, b = 255;
        sscanf(server.arg("balance").c_str(), "%d,%d,%d", &r, &g, &b);
        // Como el resto de la vista previa: en los ajustes (sin guardar), que es lo que aplica el programa
        cfg->ledBalance[0] = constrain(r, 0, 255);
        cfg->ledBalance[1] = constrain(g, 0, 255);
        cfg->ledBalance[2] = constrain(b, 0, 255);
        changes |= WEB_DISPLAY;
    }
    if (server.hasArg("luz")) {  // vista previa de la luz de ambiente
        cfg->ledMode = constrain(server.arg("luz").toInt(), 0, 4);
        cfg->ledColor = strtoul(server.arg("color").c_str(), nullptr, 16);
        cfg->ledLevel = constrain(server.arg("nivel").toInt(), 5, 100);
        cfg->ledSpeed = constrain(server.arg("vel").toInt(), 0, 2);
        changes |= WEB_DISPLAY;
    }
    if (server.hasArg("volumen")) alertTest(ALERT_SUBSCRIBER, constrain(server.arg("volumen").toInt(), 0, 100));
    if (server.hasArg("aviso")) testKind = constrain(server.arg("aviso").toInt(), 0, 3);  // lo hace el programa
}

// --- Actualizar el firmware subiendo el .bin --------------------------------

static void handleUpdatePage()
{
    String h;
    h.reserve(2500);
    h += F("<!doctype html><html lang='es'><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'><title>Actualizar firmware</title>");
    h += STYLE;
    h += F("</head><body><main><h1><b>&#9654;</b> Actualizar firmware</h1><div class='card'>"
           "<p>Versión instalada: <b>" FIRMWARE_VERSION "</b> para la placa <b>" FIRMWARE_BOARD "</b>.</p>"
           "<label for='f'>Archivo del firmware nuevo (.bin)</label><input type='file' id='f' accept='.bin'>"
           "<div class='muted'>Solo se instala si es para esta placa. Si algo falla, sigue el actual.</div>"
           "<button id='go' type='button'>Instalar</button>"
           "<p id='st' class='muted'></p><progress id='pr' max='100' value='0' style='width:100%;display:none'></progress>"
           "</div><p class='muted'><a href='/'>&larr; Volver a los ajustes</a></p></main><script>"
           "go.onclick=function(){var x=f.files[0];if(!x){st.textContent='Elige primero el archivo .bin';return}"
           "go.disabled=true;pr.style.display='block';st.textContent='Enviando… no cierres esta página.';"
           "var d=new FormData();d.append('firmware',x);var r=new XMLHttpRequest();"
           "r.upload.onprogress=function(e){if(e.lengthComputable)pr.value=e.loaded*100/e.total};"
           "r.onload=function(){st.textContent=r.responseText;if(r.status==200)"
           "setTimeout(function(){location.href='/'},15000);else go.disabled=false};"
           "r.onerror=function(){st.textContent='Se cortó la conexión. Vuelve a intentarlo.';go.disabled=false};"
           "r.open('POST','/actualizar?size='+x.size);r.send(d)}"
           "</script></body></html>");
    server.send(200, "text/html; charset=utf-8", h);
}

// Va recibiendo el archivo por trozos y escribiéndolo en el hueco libre
static void handleUpdateUpload()
{
    HTTPUpload &u = server.upload();
    static int lastShown = -1;
    if (u.status == UPLOAD_FILE_START) {
        updateError = "";
        lastShown = -1;
        Serial.printf("Actualización: recibiendo %s\n", u.filename.c_str());
        if (!updaterBegin(server.arg("size").toInt())) updateError = "No hay sitio para el firmware nuevo";
        screenProgress("Actualizando el contador", "Recibiendo por WiFi...", 0);
    } else if (u.status == UPLOAD_FILE_WRITE) {
        if (updateError.length()) return;
        if (!updaterWrite(u.buf, u.currentSize)) {
            updateError = "Error al escribir el firmware";
            updaterAbort();
            return;
        }
        int pct = updaterProgress();
        if (pct / 5 != lastShown) {
            lastShown = pct / 5;
            screenProgress("Actualizando el contador", "Recibiendo por WiFi...", pct);
        }
    } else if (u.status == UPLOAD_FILE_END) {
        if (!updateError.length()) updaterEnd(updateError);
    } else if (u.status == UPLOAD_FILE_ABORTED) {
        updaterAbort();
        updateError = "Se cortó la subida";
    }
}

static void handleUpdateDone()
{
    if (updateError.length()) {
        Serial.printf("Actualización: rechazada (%s)\n", updateError.c_str());
        screenMessage("Actualización no instalada", updateError + ".", "Sigue el firmware actual.");
        server.send(400, "text/plain; charset=utf-8", "No se ha instalado: " + updateError + ".");
        return;
    }
    Serial.println("Actualización: hecha, reiniciando");
    screenProgress("Actualización terminada", "Reiniciando...", 100);
    server.send(200, "text/plain; charset=utf-8",
                "Instalado. El contador se reinicia; esta página vuelve a los ajustes en unos segundos.");
    restartAt = millis() + 1500;
}

// Descarga del historial de la tarjeta SD
static void handleHistory()
{
    if (!sdHasHistory()) {
        server.send(404, "text/plain; charset=utf-8", "No hay historial en la tarjeta.");
        return;
    }
    File f = SD.open(sdHistoryPath());
    server.sendHeader("Content-Disposition", "attachment; filename=historial.csv");
    server.streamFile(f, "text/csv; charset=utf-8");
    f.close();
}

String webuiHost() { return host; }

void webuiBegin(Settings *settings, const ChannelStats *stats)
{
    cfg = settings;
    data = stats;
    if (running) return;
    if (!host.length()) {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char name[24];
        snprintf(name, sizeof(name), "contador-%02x%02x", mac[4], mac[5]);
        host = name;
    }
    if (MDNS.begin(host.c_str())) MDNS.addService("http", "tcp", 80);
    static bool routes = false;
    if (!routes) {
        server.on("/", HTTP_GET, []() { sendPage(); });
        server.on("/guardar", HTTP_POST, handleSave);
        server.on("/probar", HTTP_GET, handlePreview);
        server.on("/historial.csv", HTTP_GET, handleHistory);
        server.on("/actualizar", HTTP_GET, handleUpdatePage);
        server.on("/actualizar", HTTP_POST, handleUpdateDone, handleUpdateUpload);
        server.onNotFound([]() { server.sendHeader("Location", "/"); server.send(302); });
        routes = true;
    }
    server.begin();
    running = true;
    Serial.printf("Ajustes: http://%s.local o http://%s\n", host.c_str(), WiFi.localIP().toString().c_str());
}

void webuiStop()
{
    if (!running) return;
    server.stop();
    MDNS.end();
    running = false;
}

void webuiLoop()
{
    if (running) server.handleClient();
    if (restartAt && (int32_t)(millis() - restartAt) >= 0) ESP.restart();
}

int webuiTakeTest()
{
    int t = testKind;
    testKind = -1;
    return t;
}

uint8_t webuiTakeChanges()
{
    uint8_t c = changes;
    changes = 0;
    return c;
}

#endif
