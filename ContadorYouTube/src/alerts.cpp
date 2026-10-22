#include "alerts.h"
#include "../config.h"

#if defined(BOARD_CYD) && !defined(LILYGO_T5_EPD47_S3)

#include <SD.h>
#include "sdcard.h"

// Freenove FNK0114F (esquema oficial): LED RGB de ánodo común (se enciende
// con LOW) y amplificador SC8002B alimentado por el DAC del GPIO26, que se
// activa con GPIO4 a LOW (una resistencia lo mantiene apagado y sin ruido).
#define LED_R 22
#define LED_G 16
#define LED_B 17
#define SPEAKER_DAC 26
#define AUDIO_EN 4
#define SAMPLE_RATE 16000

// Canales LEDC. Con el núcleo 3 (Arduino IDE) LovyanGFX toma la luz de la
// pantalla con ledcAttach(), que ocupa el primer canal libre (el 0): por eso
// aquí se deja que el núcleo elija también canales libres. Si se fijaran el 0,
// 1 y 2, el rojo compartiría señal con la luz de la pantalla (se veía siempre
// encendido y no seguía al color). Con el núcleo 2 (PlatformIO), la pantalla
// usa el canal 7 y estos fijos no chocan.
static const uint8_t LED_PINS[3] = {LED_R, LED_G, LED_B};
static const uint8_t LED_CHANNELS[3] = {0, 1, 2};

static uint8_t volume = 50;
static bool ledOn = true, quiet = false, unread = false, offline = false;

// Luz de ambiente
static uint8_t ambMode = LED_BREATHE, ambLevel = 30, ambSpeed = 1;
static uint8_t ambColor[3] = {255, 0, 51};
static bool offlineBlink = true;
static int trend = -1;

// Animación en curso
enum Anim { ANIM_NONE, ANIM_FLASH, ANIM_RAINBOW };
static Anim anim = ANIM_NONE;
static uint8_t animColor[3];
static uint32_t animStart = 0, animLength = 0;

bool alertsAvailable() { return true; }

// Balance de blancos del LED: en esta placa el azul luce mucho más que el
// rojo y el verde, así que se atenúa (y algo el rojo) para que el blanco salga
// blanco y las mezclas se parezcan al color elegido. Valores medidos con una
// webcam (luz en lineal, con el LED apagado restado). Lo fija el programa con
// el valor guardado en los ajustes.
static uint8_t balance[3] = {192, 255, 82};

void alertsSetBalance(uint8_t r, uint8_t g, uint8_t b)
{
    balance[0] = r;
    balance[1] = g;
    balance[2] = b;
}

// PWM de 12 bits: 4096 pasos, para que los tonos tenues y el "respirar" sean
// suaves. Ánodo común: el LED luce con el pin a LOW, así que apagado es el
// ciclo completo a HIGH (4096). Con 255 de 256 quedaba un pulso a LOW en cada
// ciclo y el rojo, el que más luce con tan poca corriente, se veía siempre.
#define LED_BITS 12
#define LED_FULL (1 << LED_BITS)

static void setLedF(float r, float g, float b)  // 0-1 por color, antes del balance
{
    const float v[3] = {r, g, b};
    for (int i = 0; i < 3; i++) {
        float x = constrain(v[i], 0.0f, 1.0f) * balance[i] / 255.0f;
        uint32_t duty = LED_FULL - (uint32_t)lroundf(x * LED_FULL);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(LED_PINS[i], duty);
#else
        ledcWrite(LED_CHANNELS[i], duty);
#endif
    }
}

static void setLed(uint8_t r, uint8_t g, uint8_t b) { setLedF(r / 255.0f, g / 255.0f, b / 255.0f); }

void alertsInit()
{
    for (int i = 0; i < 3; i++) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        if (!ledcAttach(LED_PINS[i], 5000, LED_BITS))
            Serial.printf("LED: no se pudo usar el GPIO%d\n", LED_PINS[i]);
#else
        ledcSetup(LED_CHANNELS[i], 5000, LED_BITS);
        ledcAttachPin(LED_PINS[i], LED_CHANNELS[i]);
#endif
    }
    setLed(0, 0, 0);
    pinMode(AUDIO_EN, OUTPUT);
    digitalWrite(AUDIO_EN, HIGH);  // amplificador apagado
}

void alertsConfigure(uint8_t vol, bool led, bool q)
{
    volume = vol;
    ledOn = led;
    quiet = q;
}

void alertsSetUnread(bool u) { unread = u; }

void alertsAmbient(uint8_t mode, uint32_t color, uint8_t level, uint8_t speed, bool blink)
{
    ambMode = mode;
    ambColor[0] = color >> 16;
    ambColor[1] = color >> 8;
    ambColor[2] = color;
    ambLevel = constrain(level, 5, 100);
    ambSpeed = speed > 2 ? 1 : speed;
    offlineBlink = blink;
}

void alertsSetTrend(int t) { trend = t; }
void alertsSetOffline(bool o) { offline = o; }

// --- Sonido --------------------------------------------------------------
// Notas tipo campana (seno con un poco de segundo armónico y caída
// exponencial) generadas al momento por el DAC, en una tarea del otro núcleo
// para que la pantalla siga animándose mientras suena.

struct Note {
    uint16_t freq;  // Hz; 0 = silencio
    uint16_t ms;
};

static const Note COMMENT_TUNE[] = {{784, 110}, {1047, 260}};
static const Note SUBSCRIBER_TUNE[] = {{1047, 90}, {1319, 90}, {1568, 300}};
static const Note VIDEO_TUNE[] = {{784, 110}, {784, 110}, {1175, 340}};
static const Note MILESTONE_TUNE[] = {{523, 120}, {659, 120}, {784, 120}, {1047, 220}, {0, 60}, {784, 120}, {1047, 520}};

static int8_t sine[256];

static void playTune(const Note *notes, int count, uint8_t vol)
{
    if (!vol) return;
    if (!sine[64]) {
        for (int i = 0; i < 256; i++) sine[i] = (int8_t)lroundf(127 * sinf(2 * PI * i / 256));
    }
    // Volumen percibido: curva cuadrática; el pico (fundamental + armónico) cabe en 0-255
    const float amp = 0.75f * (vol / 100.0f) * (vol / 100.0f) + 0.02f;
    dacWrite(SPEAKER_DAC, 128);
    digitalWrite(AUDIO_EN, LOW);
    delay(20);
    const uint32_t period = 1000000 / SAMPLE_RATE;
    uint32_t next = micros();
    for (int n = 0; n < count; n++) {
        const uint32_t samples = (uint32_t)notes[n].ms * SAMPLE_RATE / 1000;
        const uint32_t step = (uint32_t)((uint64_t)notes[n].freq * 4294967296ULL / SAMPLE_RATE);
        const float decay = expf(-3.0f / samples);  // cae a ~5 % al final de la nota
        const uint32_t attack = SAMPLE_RATE / 400;  // 2,5 ms sin chasquido
        uint32_t phase = 0;
        float env = 1;
        for (uint32_t i = 0; i < samples; i++) {
            int v = 128;
            if (notes[n].freq) {
                float a = (i < attack ? (float)i / attack : env) * amp;
                v += (int)(a * (sine[phase >> 24] + 0.35f * sine[(phase >> 23) & 255]) / 1.35f);
                phase += step;
                if (i >= attack) env *= decay;
            }
            while ((int32_t)(micros() - next) < 0) {
            }
            dacWrite(SPEAKER_DAC, (uint8_t)constrain(v, 0, 255));
            next += period;
        }
    }
    dacWrite(SPEAKER_DAC, 128);
    delay(20);
    digitalWrite(AUDIO_EN, HIGH);
}

// Sonido propio de la tarjeta SD: WAV PCM de 8 o 16 bits, mono o estéreo,
// 8-48 kHz (mejor 16-22 kHz mono: el DAC tiene 8 bits). Como mucho 15 s.
static bool playWav(const char *path, uint8_t vol)
{
    File f = SD.open(path);
    if (!f) return false;
    char id[5] = {0};
    uint32_t size = 0, rate = 0, dataLeft = 0;
    uint16_t format = 0, channels = 0, bits = 0;
    if (f.read((uint8_t *)id, 4) != 4 || strcmp(id, "RIFF") || f.read((uint8_t *)&size, 4) != 4 ||
        f.read((uint8_t *)id, 4) != 4 || strcmp(id, "WAVE")) {
        f.close();
        return false;
    }
    while (f.available() >= 8) {  // trozos: "fmt " con el formato y "data" con el sonido
        f.read((uint8_t *)id, 4);
        f.read((uint8_t *)&size, 4);
        if (!strcmp(id, "fmt ")) {
            uint8_t fmt[16];
            if (size < 16 || f.read(fmt, 16) != 16) break;
            format = fmt[0] | fmt[1] << 8;
            channels = fmt[2] | fmt[3] << 8;
            rate = fmt[4] | fmt[5] << 8 | fmt[6] << 16 | (uint32_t)fmt[7] << 24;
            bits = fmt[14] | fmt[15] << 8;
            f.seek(f.position() + size - 16 + (size & 1));
        } else if (!strcmp(id, "data")) {
            dataLeft = size;
            break;
        } else {
            f.seek(f.position() + size + (size & 1));
        }
    }
    if (format != 1 || (bits != 8 && bits != 16) || channels < 1 || channels > 2 || rate < 8000 || rate > 48000 ||
        !dataLeft) {
        Serial.printf("Sonido %s: formato no admitido (usa WAV PCM de 8 o 16 bits)\n", path);
        f.close();
        return false;
    }
    const uint32_t frame = channels * bits / 8;
    dataLeft = min(dataLeft, rate * frame * 15);
    float amp = (vol / 100.0f) * (vol / 100.0f) + 0.02f;
    if (amp > 1) amp = 1;
    dacWrite(SPEAKER_DAC, 128);
    digitalWrite(AUDIO_EN, LOW);
    delay(20);
    const uint32_t period = 1000000 / rate;
    uint32_t next = micros();
    static uint8_t buf[512];
    while (dataLeft >= frame) {
        int n = f.read(buf, min<uint32_t>(sizeof(buf) / frame * frame, dataLeft));
        if (n <= 0) break;
        dataLeft -= n;
        for (int i = 0; i + (int)frame <= n; i += frame) {
            int v = 0;  // -128..127
            for (int c = 0; c < channels; c++)
                v += bits == 8 ? (int)buf[i + c] - 128 : (int8_t)buf[i + c * 2 + 1];
            v /= channels;
            while ((int32_t)(micros() - next) < 0) {
            }
            dacWrite(SPEAKER_DAC, (uint8_t)constrain(128 + (int)(v * amp), 0, 255));
            next += period;
        }
    }
    f.close();
    dacWrite(SPEAKER_DAC, 128);
    delay(20);
    digitalWrite(AUDIO_EN, HIGH);
    return true;
}

static volatile bool playing = false;
static char jobPath[40];
static const Note *jobNotes;
static int jobCount;
static uint8_t jobVolume;

static void soundTask(void *)
{
    if (!jobPath[0] || !playWav(jobPath, jobVolume)) playTune(jobNotes, jobCount, jobVolume);
    playing = false;
    vTaskDelete(nullptr);
}

static void playKind(AlertKind kind, uint8_t vol)
{
    if (!vol || playing) return;  // si ya suena otro aviso, este va solo con luz
    switch (kind) {
    case ALERT_COMMENT: jobNotes = COMMENT_TUNE, jobCount = 2; break;
    case ALERT_SUBSCRIBER: jobNotes = SUBSCRIBER_TUNE, jobCount = 3; break;
    case ALERT_VIDEO: jobNotes = VIDEO_TUNE, jobCount = 3; break;
    case ALERT_MILESTONE: jobNotes = MILESTONE_TUNE, jobCount = 7; break;
    }
    jobVolume = vol;
    String custom = sdSoundPath(kind);  // sonido propio en la tarjeta, si lo hay
    strlcpy(jobPath, custom.c_str(), sizeof(jobPath));
    playing = true;
    // Núcleo 0 (el de la WiFi, que tiene más prioridad); el programa sigue en el 1
    if (xTaskCreatePinnedToCore(soundTask, "sonido", 3072, nullptr, 2, nullptr, 0) != pdPASS) playing = false;
}

// --- LED -----------------------------------------------------------------

static void startAnim(AlertKind kind)
{
    animStart = millis();
    anim = ANIM_FLASH;
    animLength = 1500;
    switch (kind) {
    case ALERT_COMMENT: animColor[0] = 0, animColor[1] = 60, animColor[2] = 255; break;
    case ALERT_SUBSCRIBER: animColor[0] = 0, animColor[1] = 255, animColor[2] = 40; break;
    case ALERT_VIDEO: animColor[0] = 255, animColor[1] = 0, animColor[2] = 20; break;
    case ALERT_MILESTONE:
        anim = ANIM_RAINBOW;
        animLength = 8000;
        break;
    }
}

void alertPlay(AlertKind kind)
{
    if (quiet) return;
    if (ledOn) startAnim(kind);
    playKind(kind, volume);
}

void alertTest(AlertKind kind, uint8_t vol)
{
    if (ledOn) startAnim(kind);
    playKind(kind, vol);
}

// Rueda de color: 0-255 -> rojo, verde, azul
static void wheel(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b)
{
    if (pos < 85) r = 255 - pos * 3, g = pos * 3, b = 0;
    else if (pos < 170) pos -= 85, r = 0, g = 255 - pos * 3, b = pos * 3;
    else pos -= 170, r = pos * 3, g = 0, b = 255 - pos * 3;
}

// Color de la luz de ambiente en este momento
static void ambient(float &r, float &g, float &b)
{
    static const uint16_t BREATHE_MS[] = {6000, 4000, 2500}, RAINBOW_MS[] = {40000, 20000, 8000};
    const uint32_t now = millis();
    uint8_t c[3] = {ambColor[0], ambColor[1], ambColor[2]};
    float k = 1;  // 0-1: brillo dentro del efecto
    switch (ambMode) {
    case LED_OFF: r = g = b = 0.0f; return;
    case LED_FIXED: break;
    case LED_BREATHE: k = 0.12f + 0.88f * (0.5f - 0.5f * cosf(2 * PI * (now % BREATHE_MS[ambSpeed]) / BREATHE_MS[ambSpeed])); break;
    case LED_RAINBOW: wheel((uint8_t)((now % RAINBOW_MS[ambSpeed]) * 256 / RAINBOW_MS[ambSpeed]), c[0], c[1], c[2]); break;
    case LED_CHANNEL:  // verde si hoy suben las vistas, ámbar si no; respira despacio
        if (trend > 0) c[0] = 0, c[1] = 255, c[2] = 40;
        else if (trend == 0) c[0] = 255, c[1] = 110, c[2] = 0;
        k = 0.12f + 0.88f * (0.5f - 0.5f * cosf(2 * PI * (now % BREATHE_MS[0]) / BREATHE_MS[0]));
        break;
    }
    // El ojo ve el brillo de forma no lineal: curva cuadrática para la intensidad y el efecto
    float level = ambLevel / 100.0f, scale = level * level * k * k / 255.0f;
    r = c[0] * scale;
    g = c[1] * scale;
    b = c[2] * scale;
}

void alertsLoop()
{
    static uint32_t last = 0;
    if (millis() - last < 15) return;
    last = millis();
    const uint32_t now = millis(), t = now - animStart;
    if (anim != ANIM_NONE && t >= animLength) anim = ANIM_NONE;
    if (quiet) {  // de noche, nada
        anim = ANIM_NONE;
        setLed(0, 0, 0);
        return;
    }
    if (anim == ANIM_FLASH) {  // los avisos mandan
        bool on = (t / 250) % 2 == 0;  // 3 destellos
        setLed(on ? animColor[0] : 0, on ? animColor[1] : 0, on ? animColor[2] : 0);
        return;
    }
    if (anim == ANIM_RAINBOW) {
        uint8_t r, g, b;
        wheel((uint8_t)(t / 6), r, g, b);
        setLed(r, g, b);
        return;
    }
    // Encima del ambiente, señales breves: sin conexión, un parpadeo ámbar cada
    // 10 s; comentarios sin ver, doble pulso azul cada 4 s
    uint32_t p10 = now % 10000, p4 = now % 4000;
    if (offline && offlineBlink && p10 < 150) return setLed(255, 110, 0);
    if (unread && (p4 < 120 || (p4 >= 260 && p4 < 380))) return setLed(0, 60, 255);
    float r, g, b;
    ambient(r, g, b);
    setLedF(r, g, b);
}

#else

// Placas sin LED ni altavoz
bool alertsAvailable() { return false; }
void alertsInit() {}
void alertsConfigure(uint8_t, bool, bool) {}
void alertPlay(AlertKind) {}
void alertTest(AlertKind, uint8_t) {}
void alertsSetUnread(bool) {}
void alertsSetOffline(bool) {}
void alertsAmbient(uint8_t, uint32_t, uint8_t, uint8_t, bool) {}
void alertsSetTrend(int) {}
void alertsSetBalance(uint8_t, uint8_t, uint8_t) {}
void alertsLoop() {}

#endif
