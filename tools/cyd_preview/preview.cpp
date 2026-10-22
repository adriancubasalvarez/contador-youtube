// Vista previa en el PC de las pantallas de la CYD: compila el mismo
// src/cyd/screen.cpp con LovyanGFX y las mismas fuentes, pero dibujando en
// una imagen en memoria (cyd_preview_hw.h). Los datos son de ejemplo.
// Escribe un PPM por pantalla en la carpeta indicada. Ver build.sh.
#define SDL_MAIN_HANDLED
#include <LovyanGFX.hpp>
#include <chrono>
#include <thread>
#include "../../ContadorYouTube/src/history.h"
#include "../../ContadorYouTube/src/screen.h"
#include "../../ContadorYouTube/src/trends.h"
#include "cyd_preview_hw.h"

// Historial de ejemplo: 14 días de vistas y un canal que crece
static const int64_t DAILY[14] = {812, 905, 760, 1120, 1480, 1310, 990, 870, 1640, 2210, 1930, 1570, 1720, 640};

void historyRecord(int32_t, int64_t, int64_t) {}
void historyClear() {}
void historyDailyViews(int32_t, int n, int64_t *out)
{
    for (int i = 0; i < n; i++) out[i] = DAILY[14 - n + i];
}
void historySubscribers(int32_t, int n, int64_t *out)
{
    for (int i = 0; i < n; i++)  // crece unos 40 al día, con algún día sin dato
        out[i] = (i % 9 == 4) ? -1 : 127100 + i * 40 + (i % 5) * 25;
}
int historySubscriberDelta(int32_t, int, int64_t, int64_t &delta)
{
    delta = 1240;
    return 7;
}

static void save(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    FILE *f = fopen(path, "wb");
    auto *img = previewFrame();
    fprintf(f, "P6\n%d %d\n255\n", (int)img->width(), (int)img->height());
    for (int y = 0; y < img->height(); y++)
        for (int x = 0; x < img->width(); x++) {
            auto c = img->readPixelRGB(x, y);
            uint8_t px[3] = {c.r, c.g, c.b};
            fwrite(px, 1, 3, f);
        }
    fclose(f);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    ChannelStats st;
    st.title = "Mi Canal de Electrónica";
    st.subscribers = 128400;
    st.views = 9876543;
    st.videos = 214;
    st.hasVideo = true;
    st.videoTitle = "Cómo hice un contador de suscriptores con un ESP32 por menos de 20 €";
    time_t now = 1792000000;  // fecha de ejemplo
    st.videoPublished = now - 3 * 86400;
    st.videoViews = 15320;
    st.videoLikes = 1210;
    st.videoComments = 87;
    int32_t today = (int32_t)(now / 86400);

    screenInit();
    screenSetup("YT-Dashboard");
    save(dir, "0_configuracion");
    screenPage(PAGE_SUBSCRIBERS, st, today, now, false);
    save(dir, "1_suscriptores");
    screenPage(PAGE_VIEWS, st, today, now, false);
    save(dir, "2_vistas");
    screenPage(PAGE_VIDEO, st, today, now, false);
    save(dir, "3_video");
    st.hasTrend = true;
    st.trendTitle = "Soldar SMD sin microscopio: trucos para empezar";
    st.trendGain = 3412;
    st.trendHours = 24;
    st.trendViews = 61540;
    st.trendTimes = 3.2f;
    st.trendEngagement = 6.4f;
    screenPage(PAGE_TREND, st, today, now, false);
    save(dir, "3b_tendencia");
    screenPage(PAGE_CHART, st, today, now, true);
    save(dir, "4_grafico");
    screenInfo("contador-1a2b", "192.168.1.57");
    save(dir, "5_ajustes");
    screenMessage("Sin conexión WiFi", "No se pudo conectar a «MiCasa». Revisa la red (2,4 GHz) y la contraseña.",
                  "Abriendo la configuración...");
    save(dir, "6_mensaje");

    // Vistas de detalle (al tocar) y comprobación de sus zonas
    int index = -1;
    screenPage(PAGE_VIDEO, st, today, now, false);
    printf("video: chip comentarios=%d titulo=%d cabecera=%d\n", screenHit(265, 175, index), screenHit(150, 100, index),
           screenHit(150, 20, index));
    screenPage(PAGE_CHART, st, today, now, false);
    TouchTarget t = screenHit(170, 150, index);
    printf("grafico: barra=%d dia=%d\n", t, index);
    VideoComment comments[3];
    comments[0].author = "@marta_makes";
    comments[0].text = "¡Qué buena idea! Lo he montado con la pantalla de 2,8\" y funciona a la primera. ¿Se podría añadir el número de directos?";
    comments[0].published = now - 2 * 3600;
    comments[0].likes = 14;
    comments[0].replies = 2;
    comments[1].author = "@Electrónica Fácil";
    comments[1].text = "Muy bien explicado, sobre todo la parte del portal WiFi.";
    comments[1].published = now - 5 * 3600;
    comments[2].author = "@pablo";
    comments[2].text = "Gracias!";
    comments[2].published = now - 26 * 3600;
    screenComments(st, comments, 3, 0, now, "");
    save(dir, "d1_comentarios");
    printf("comentarios: mas=%d siguiente=%d\n", screenHit(160, 150, index), index);
    VideoSummary videos[10];
    const char *titles[] = {"Cómo hice un contador de suscriptores con un ESP32 por menos de 20 €",
                            "Soldar SMD sin microscopio: trucos para empezar", "Mi taller en 2026",
                            "Arduino vs ESP32: ¿cuál elegir?", "Reparo una fuente de laboratorio",
                            "Osciloscopio barato: ¿merece la pena?", "Mi primera PCB", "Robot sigue-líneas",
                            "Estación meteorológica", "Tour por mi taller"};
    const int gains[] = {1210, 3412, 640, 820, 150, 95, 410, 60, 33, 12};
    for (int i = 0; i < 10; i++) {  // medidas de hace 24 h
        videos[i].id = "v" + String(i);
        videos[i].title = titles[i];
        videos[i].published = now - (3 + i * 7) * 86400;
        videos[i].views = 15320 + i * 23111 * (i % 2 + 1) - gains[i];
        videos[i].likes = videos[i].views / (12 + i);
        videos[i].comments = videos[i].views / (150 + 20 * i);
    }
    trendsRecord(videos, 10, now - 24 * 3600);
    for (int i = 0; i < 10; i++) videos[i].views += gains[i];  // y las de ahora
    trendsRecord(videos, 10, now);
    screenVideos(videos, 10, 0, false, now, "");
    save(dir, "d2_videos");
    screenVideos(videos, 10, 0, true, now, "");
    save(dir, "d2b_videos_crecen");
    screenDay(today, 9);
    save(dir, "d3_dia");
    screenSubscriberHistory(st, today);
    save(dir, "d4_suscriptores");
    screenComments(st, comments, 0, 0, now, "Los comentarios están desactivados en este vídeo.");
    save(dir, "d5_sin_comentarios");

    // Avisos: comentarios nuevos sin ver y celebración de un hito
    st.newComments = 2;
    screenPage(PAGE_VIDEO, st, today, now, false);
    save(dir, "d6_comentarios_nuevos");
    screenComments(st, comments, 3, 0, now, "");
    save(dir, "d7_comentarios_marcados");
    screenCelebrate("100.000", "suscriptores");
    for (int i = 0; i < 40; i++) {  // el confeti se anima con el tiempo real
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        screenAnimate();
    }
    save(dir, "d8_hito");

    // Efectos a mitad de animación
    st.newComments = 0;
    screenPage(PAGE_SUBSCRIBERS, st, today, now, false);
    screenGain(128300, 128400);
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    screenAnimate();
    save(dir, "d9_subida");
    screenPage(PAGE_VIEWS, st, today, now, false);
    screenToast(TOUCH_COMMENTS, "Comentario nuevo", "@marta_makes: ¡Qué buena idea! Lo he montado con la pantalla de 2,8\"");
    std::this_thread::sleep_for(std::chrono::milliseconds(380));
    screenAnimate();
    save(dir, "e1_aviso_comentario");
    printf("aviso: zona=%d\n", screenHit(160, 30, index));

    // Cabecera con batería (y datos sin actualizar)
    st.battery = 83;
    screenPage(PAGE_SUBSCRIBERS, st, today, now, true);
    save(dir, "e2_bateria");
    st.battery = 9;
    screenPage(PAGE_VIEWS, st, today, now, false);
    save(dir, "e3_bateria_baja");
    return 0;
}
