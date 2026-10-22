"""Genera el manual en PDF del contador de YouTube (pantalla táctil CYD y OLED).

    python tools/manual/build_manual.py docs/manual-contador-youtube.pdf

Usa las imágenes de docs/: pantallas OLED (tools/oled_preview), pantallas de la
CYD (tools/cyd_preview + cyd_screens.py), capturas del instalador
(installer_screens.py), conexiones (wiring.py) y capturas de Google Cloud.
"""
import sys

from manual_common import *  # noqa: F401,F403  estilos y utilidades
from reportlab.lib.units import cm
from reportlab.platypus import CondPageBreak, PageBreak, SimpleDocTemplate, Spacer, Table, TableStyle

OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else DOCS / "manual-contador-youtube.pdf"

TACTIL = badge("PANTALLA TÁCTIL")

# --------------------------------------------------------------- contenido ---

story = []

# Portada
story += [
    Spacer(1, 1.0 * cm),
    P("MANUAL DE MONTAJE Y USO", "kicker"),
    Spacer(1, 6),
    P("Contador de YouTube de escritorio", "cover_title"),
    Spacer(1, 8),
    P("Un pequeño aparato con ESP32 que muestra en tiempo real los suscriptores, las vistas, el último vídeo y "
      "los comentarios de tu canal. Dos modelos: con pantalla táctil a color o con pantalla OLED.", "cover_sub"),
    Spacer(1, 0.8 * cm),
    grid(["img/cyd-1_suscriptores.png", "img/oled-1_suscriptores.png"], 7.6 * cm,
         captions=["Pantalla táctil a color (2,8\")", "Contador OLED (0,96\")"]),
    Spacer(1, 0.6 * cm),
]
facts = Table([[P("<b>≈ 17 €</b><br/>táctil · ≈ 12 € OLED", "cell"), P("<b>≈ 15 min</b><br/>con el instalador", "cell"),
                P("<b>Sin soldar</b><br/>ni programar", "cell"), P("<b>Se actualiza</b><br/>sin cables", "cell")]],
              colWidths=[CONTENT_W / 4] * 4)
facts.setStyle(TableStyle([
    ("LINEABOVE", (0, 0), (-1, 0), 1.2, INK), ("LINEBELOW", (0, 0), (-1, 0), 0.5, LINE),
    ("TOPPADDING", (0, 0), (-1, -1), 10), ("BOTTOMPADDING", (0, 0), (-1, -1), 10),
]))
story += [facts, Spacer(1, 1 * cm),
          P("Versión 2.1 del manual · octubre de 2026 · firmware 1.6.0 · sketch <b>ContadorYouTube</b> · "
            "<i>English manual: manual-youtube-counter.pdf</i>", "small"),
          P("© 2026 Adrian Cubas. Manual con licencia CC BY-NC 4.0; código y firmware con licencia PolyForm "
            "Noncommercial 1.0.0: puedes montarlo, modificarlo y compartirlo sin fines comerciales. Para "
            "venderlo, pide permiso al autor. Detalles en el anexo A.", "small"),
          PageBreak()]

# Índice
story += [P("Contenido", "h1")]
toc = [
    ("Qué vas a construir", ""), ("Material", ""),
    ("Paso 1 · Crear tu clave de la API de YouTube", "10 min"),
    ("Paso 2 · Instalarlo con el instalador de Windows (recomendado)", "5 min"),
    ("Paso 2B · Alternativa: instalarlo con Arduino IDE", "30 min"),
    ("Paso 3 · Otras formas de configurarlo: móvil o tarjeta microSD", ""),
    ("Paso 4 · Uso diario del contador OLED", ""),
    ("Paso 5 · Uso diario de la pantalla táctil", ""),
    ("Paso 6 · Avisos, efectos y luz de ambiente (táctil)", ""),
    ("Paso 7 · Tarjeta microSD, sonidos y batería (táctil)", ""),
    ("Paso 8 · Ajustes desde el navegador", ""),
    ("Paso 9 · Actualizar el firmware", "2 min"),
    ("Paso 10 · Colocarlo en el escritorio (opcional)", ""),
    ("Lista de pruebas", ""), ("Solución de problemas", ""), ("Cómo funciona por dentro", ""),
    ("Notas para el Instructable", ""),
    ("Anexo A · Publicar el firmware y el instalador", ""),
    ("Anexo B · Versión con pantalla de tinta electrónica", ""),
]
story += [table([[t, d] for t, d in toc], [CONTENT_W - 3 * cm, 3 * cm], header=False),
          Spacer(1, 10),
          P(f"Los apartados marcados con {TACTIL} solo se aplican al modelo con pantalla táctil.", "small"),
          PageBreak()]

# Qué vas a construir
story += [P("Qué vas a construir", "h1"),
          P("Un contador para el escritorio que se conecta a tu WiFi y consulta cada pocos minutos los datos "
            "<b>públicos</b> de tu canal de YouTube. Funciona solo, enchufado a cualquier cargador de móvil, y va "
            "rotando entre cinco vistas. Hay dos modelos con el mismo programa:"),
          table([
              ["", "Pantalla táctil a color", "Contador OLED"],
              ["Pantalla", "2,8\", 320×240, a color, táctil", "0,96\", 128×64, amarilla y azul"],
              ["Montaje", "Ninguno: la placa ya lleva la pantalla", "4 cables, sin soldar"],
              ["Precio aprox.", "≈ 17 €", "≈ 12 €"],
              ["Al tocar", "Comentarios, tus 10 últimos vídeos, vistas de un día, suscriptores en 30 días", "—"],
              ["Avisos", "Efectos en pantalla, LED de color y sonido", "—"],
              ["Extras", "Tarjeta microSD (historial, sonidos, configuración), batería", "—"],
              ["Idioma", "Español o inglés (pantalla, ajustes y avisos)", "Español o inglés"],
          ], [3 * cm, (CONTENT_W - 3 * cm) / 2, (CONTENT_W - 3 * cm) / 2]),
          Spacer(1, 10),
          grid(["img/cyd-1_suscriptores.png", "img/cyd-2_vistas.png", "img/cyd-3_video.png", "img/cyd-4_grafico.png"],
               5.4 * cm, captions=["Suscriptores y su cambio en 7 días", "Vistas totales y las de hoy",
                                   "Último vídeo: vistas, «me gusta» y comentarios", "Vistas por día (2 semanas)"]),
          P("Cuatro de las cinco vistas de la pantalla táctil (la quinta, «el que más crece», está en el paso 5; "
            "el contador OLED muestra lo mismo en pequeño). Imágenes "
            "generadas con el mismo código que usa el aparato; los datos son de ejemplo.", "caption"),
          tip("La API pública de YouTube no da las vistas por día. El contador las calcula guardando un registro "
              "diario, así que el gráfico, «hoy +…» y el crecimiento aparecen tras 1-2 días de uso.",
              "YouTube redondea los suscriptores a 3 cifras (128.400, no 128.437): el número cambia cuando cambia "
              "la tercera cifra."),
          PageBreak()]

# Material
story += [P("Material", "h1"),
          P("Pantalla táctil a color", "h2"),
          table([
              ["Pieza", "Detalle", "Precio aprox."],
              ["Placa con pantalla 2,8\"", "Freenove «ESP32 CYD 2.8\" … <b>ILI9341</b> TN Touch» (FNK0114F), con el "
                                         "ESP32 integrado. Otras «Cheap Yellow Display» ILI9341 deberían valer, "
                                         "pero esta es la probada.", "17 €"],
              ["Cable USB-C <b>de datos</b>", "Para instalarlo y alimentarlo", "—"],
              ["Cargador USB de 5 V", "Cualquier cargador de móvil (1 A)", "—"],
          ], [4 * cm, CONTENT_W - 6.5 * cm, 2.5 * cm]),
          Spacer(1, 4),
          P("Opcional para la pantalla táctil", "h2"),
          table([
              ["Pieza", "Para qué"],
              ["Tarjeta microSD", "Historial completo, sonidos propios y configurar sin móvil. Mejor de 32 GB o "
                                  "menos (vienen en FAT32); las más grandes hay que prepararlas (paso 7)."],
              ["Altavoz de 8 Ω, 1-3 W", "Sonido de los avisos. Con conector JST de 1,25 mm y 2 pines (conector "
                                       "«Speaker» de la placa)."],
              ["Batería LiPo de 3,7 V", "Usarlo sin cable un rato. Conector JST de 1,25 mm (conector «BAT»); la "
                                       "placa la carga por USB."],
          ], [4 * cm, CONTENT_W - 4 * cm]),
          Spacer(1, 4),
          P("Contador OLED", "h2"),
          table([
              ["Pieza", "Detalle", "Precio aprox."],
              ["Placa ESP32", "ESP32-WROOM-32 con USB-C y chip CH340 (p. ej. Diymore NodeMCU, "
                              "pack de 2 por 15,98 €)", "8 €"],
              ["Pantalla OLED 0,96\"", "128×64, SSD1306, I2C de 4 pines, amarilla y azul, con 4 cables dupont "
                                      "hembra-hembra (p. ej. ELEGOO, pack de 3 por 11,99 €)", "4 €"],
          ], [4 * cm, CONTENT_W - 6.5 * cm, 2.5 * cm]),
          Spacer(1, 4),
          P("Además", "h2"),
          *bullets("Un ordenador con <b>Windows 10 u 11</b> para el instalador (o Windows, macOS o Linux con "
                   "Arduino IDE).",
                   "Una cuenta de Google para crear la clave de la API (gratis, sin tarjeta).",
                   "Una red WiFi de <b>2,4 GHz</b>: el ESP32 no funciona con redes de 5 GHz.",
                   "Un cable USB-C <b>de datos</b>: muchos cables baratos son «solo carga». Si el ordenador no "
                   "detecta la placa, prueba primero con otro cable."),
          PageBreak()]

# Paso 1  (recorrido verificado en la consola de Google Cloud, octubre de 2026)
story += step_title(1, "Crear tu clave de la API de YouTube", "10 min")
W1 = CONTENT_W * 0.78
story += [P("El contador lee los datos públicos de YouTube a través de la <b>YouTube Data API v3</b>, que "
            "necesita una clave gratuita. <b>Cada persona crea la suya</b> con su cuenta de Google: no cuesta "
            "nada, no pide tarjeta y se hace una sola vez. Hazla antes de instalarlo, porque el paso 2 te la pide."),
          P("1.1 · Crear un proyecto", "h2"),
          *steps("Entra en <b>console.cloud.google.com</b> con tu cuenta de Google y acepta las condiciones "
                 "si es la primera vez.",
                 "Arriba a la izquierda, junto al logo de Google Cloud, pulsa el botón con el nombre del proyecto "
                 "actual (si es tu primera vez, pondrá «Selecciona un proyecto»)."),
          image("img/gcloud-selector-proyecto.png", W1),
          *steps("En la ventana que se abre, pulsa <b>Proyecto nuevo</b>, arriba a la derecha.", start=3),
          image("img/gc-01-selector.png", W1),
          *steps("Escribe el nombre <b>contador-youtube</b>. Deja «Recurso superior» como está (<i>Sin "
                 "organización</i>) y pulsa <b>Crear</b>.", start=4),
          image("img/gc-03-nombre.png", W1),
          *steps("Al crearlo, <b>la consola sigue mostrando el proyecto anterior</b>. Abre el aviso que aparece "
                 "en la campana de notificaciones (arriba a la derecha) y pulsa <b>Seleccionar proyecto</b>. "
                 "Comprueba que el botón de arriba ya pone <b>contador-youtube</b>.", start=5),
          image("img/gc-04-notificacion.png", CONTENT_W * 0.55),
          P("1.2 · Habilitar la YouTube Data API v3", "h2"),
          *steps("Abre el menú principal (las tres rayas, arriba a la izquierda) → <b>APIs y servicios</b> → "
                 "<b>Biblioteca</b>. Busca <b>YouTube Data API v3</b> y ábrela.",
                 "Pulsa <b>Habilitar</b> y espera unos segundos.", start=6),
          image("img/gc-05-api-habilitar.png", W1),
          *steps("Se abre la página de la API. En <b>Estado</b> debe poner <b>Habilitada</b>.", start=8),
          image("img/gc-06-api-habilitada.png", CONTENT_W),
          P("1.3 · Crear la clave", "h2"),
          *steps("En el menú lateral pulsa <b>Credenciales</b> y después <b>+ Crear credenciales</b>. El aviso "
                 "amarillo sobre la «pantalla de consentimiento de OAuth» no afecta a este proyecto: ignóralo.",
                 start=9),
          image("img/gc-07-credenciales.png", CONTENT_W),
          *steps("Elige <b>Clave de API</b>.", start=10),
          image("img/gc-08-menu-clave.png", CONTENT_W * 0.6),
          *steps("Se abre el panel <b>Crear clave de API</b>. Ponle de nombre <b>contador-youtube</b>. En "
                 "<b>Elige las restricciones de API</b>, escribe «YouTube Data» en el filtro, marca "
                 "<b>YouTube Data API v3</b> y pulsa <b>Aceptar</b>.", start=11),
          image("img/gc-10-restriccion.png", CONTENT_W * 0.55),
          *steps("Comprueba que pone <b>1 API</b> y que en <b>Restricciones de aplicaciones</b> está marcado "
                 "<b>Ninguno</b> (el contador no tiene una dirección fija). Pulsa <b>Crear</b>.", start=12),
          image("img/gc-11-crear-clave.png", CONTENT_W * 0.45),
          *steps("Aparece <b>Se creó la clave de API</b>. Pulsa el botón de copiar, a la derecha de la clave "
                 "(empieza por <b>AIza</b>), y pégala en un bloc de notas para el paso 2. Luego pulsa "
                 "<b>Cerrar</b>.", start=13),
          image("img/gc-12-clave-creada.png", CONTENT_W * 0.45, "La clave está tapada en esta captura."),
          tip("No hace falta ninguna tarjeta: el uso gratuito incluye 10.000 unidades al día y un contador gasta "
              "unas 1.100. Si se superase, la API deja de responder hasta el día siguiente, pero nunca cobra.",
              "Si pierdes la clave, puedes volver a verla en <b>Credenciales</b> → <b>Claves de API</b> → "
              "<b>Mostrar clave</b>."),
          warn("Trata la clave como una contraseña: no la compartas ni la publiques. Tras crearla puede tardar "
               "unos minutos en funcionar.",
               "No actives la facturación en este proyecto y deja la clave restringida a la YouTube Data API v3: "
               "así nunca puede generar gastos.")]

# Paso 2  (verificado: instalación completa en una FNK0114F con el instalador, firmware 1.3.0)
story += [PageBreak()]
story += step_title(2, "Instalarlo con el instalador de Windows", "5 min")
IW = 12 * cm
story += [P("El instalador lo hace todo por el cable USB: graba el programa en la placa y le envía tu WiFi, tu "
            "canal y tu clave. No hace falta Arduino IDE ni el móvil."),
          *steps("Descarga <b>InstaladorContadorYouTube.exe</b> y ábrelo con doble clic. No hay que instalar "
                 "nada más: el programa del contador va dentro.",
                 "Si Windows muestra <b>«Windows protegió su PC»</b>, pulsa <b>Más información</b> → "
                 "<b>Ejecutar de todas formas</b>. Sale porque el instalador aún no está firmado con un "
                 "certificado de empresa."),
          warn("Ejecuta solo el instalador descargado de la página oficial del proyecto."),
          *steps("<b>Elige tu modelo</b> (pantalla táctil u OLED) y pulsa <b>Siguiente</b>. Arriba a la derecha "
                 "puedes cambiar el idioma del instalador (<b>Español</b> o <b>English</b>); sale en el de tu "
                 "Windows.", start=3),
          image("img/instalador-1_placa.png", IW),
          *steps("<b>Conecta el contador</b> al ordenador con el cable USB. La ventana lo detecta sola y dice "
                 "«Placa lista en COM…». Pulsa <b>Instalar el contador</b>.", start=4),
          image("img/instalador-2_conectar.png", IW),
          tip("Si no aparece, prueba otro cable o puerto. Si es la primera vez, Windows puede necesitar el "
              "controlador <b>CH340</b>: el instalador muestra un botón para descargarlo.",
              "Si el contador ya tenía el programa, ofrece <b>Solo cambiar la configuración</b>, sin "
              "reinstalar."),
          *steps("Espera a que termine la barra (menos de un minuto). No desconectes el cable.", start=5),
          image("img/instalador-3_grabando.png", IW),
          *steps("<b>Rellena tus datos</b>: elige tu red WiFi (tiene que ser de 2,4 GHz; si tu ordenador está en "
                 "la de 5 GHz, la lista propone la otra), su contraseña, tu canal (el @nombre de YouTube o el "
                 "enlace), la clave del paso 1 y el <b>idioma del contador</b> (español o inglés). Pulsa "
                 "<b>Comprobar y enviar</b>.", start=6),
          image("img/instalador-4_configurar.png", IW),
          P("El instalador consulta YouTube desde tu ordenador antes de enviar nada: si la clave o el canal "
            "están mal, te dice qué pasa (clave mal copiada, API sin habilitar, canal que no existe…)."),
          *steps("Mira cómo se marcan los tres pasos: envía la configuración por el cable, el contador se "
                 "conecta a tu WiFi y consulta YouTube.", start=7),
          image("img/instalador-5_enviando.png", IW),
          *steps("<b>¡Listo!</b> Anota la dirección de su página de ajustes (<b>contador-xxxx.local</b>) o pulsa "
                 "<b>Crear acceso directo en el escritorio</b>. Ya puedes desenchufarlo del ordenador y "
                 "enchufarlo a un cargador.", start=8),
          image("img/instalador-6_listo.png", IW),
          tip("El nombre <b>contador-xxxx.local</b> no cambia. La dirección numérica (192.168…) la reparte el "
              "router y puede cambiar; si quieres que sea siempre la misma, resérvala en tu router "
              "(«DHCP estática» o «reserva de IP»), con la dirección MAC que sale en tu router.")]

# Paso 2B  (verificado con el núcleo esp32 3.3.12 y las librerías de octubre de 2026)
story += step_title("2B", "Alternativa: instalarlo con Arduino IDE", "30 min")
story += [P("Para quien quiera compilar el código (o no use Windows). Al terminar se configura con el móvil "
            "(paso 3)."),
          P("2B.1 · Preparar Arduino IDE", "h2"),
          *steps("Descarga <b>Arduino IDE 2</b> desde <b>arduino.cc/en/software</b> e instálalo. Ábrelo.",
                 "Abre el <b>Gestor de placas</b> (segundo icono de la barra lateral izquierda), busca "
                 "<b>esp32</b> y pulsa <b>Instalar</b> en <b>esp32 de Espressif Systems</b>. Tarda varios minutos."),
          warn("No elijas «Arduino ESP32 Boards»: esa es solo para las placas Arduino Nano ESP32."),
          *steps("Abre el <b>Gestor de bibliotecas</b> (tercer icono, unos libros) e instala estas. Si pregunta "
                 "por dependencias, pulsa <b>Instalar todo</b>:", start=3),
          table([
              ["Buscar", "Instalar la de", "Modelo", "Para qué sirve"],
              ["WiFiManager", "tzapu", "Los dos", "La red YT-Dashboard y la configuración"],
              ["ArduinoJson", "Benoit Blanchon", "Los dos", "Leer las respuestas de YouTube"],
              ["LovyanGFX", "lovyan03", "Táctil", "Dibujar en la pantalla a color"],
              ["U8g2", "oliver", "OLED", "Dibujar en la pantalla OLED"],
          ], [3 * cm, 3.4 * cm, 2 * cm, CONTENT_W - 8.4 * cm]),
          Spacer(1, 6),
          P("2B.2 · Conectar la pantalla (solo OLED)", "h2"),
          P("La pantalla táctil ya viene montada: salta este apartado. Para la OLED, con la placa "
            "<b>desconectada del USB</b>, une los 4 pines:"),
          image("img/conexiones.png", CONTENT_W * 0.85),
          table([
              ["Pin de la OLED", "Pin del ESP32", "Color sugerido"],
              ["GND", "GND (cualquiera)", "Negro"],
              ["VCC", "3V3", "Rojo"],
              ["SCL", "GPIO 22 (rotulado D22 o 22)", "Amarillo"],
              ["SDA", "GPIO 21 (rotulado D21 o 21)", "Azul"],
          ], [4 * cm, 7 * cm, CONTENT_W - 11 * cm]),
          Spacer(1, 6),
          warn("Lee los nombres impresos en tu pantalla: en algunos modelos el orden es VCC-GND en lugar de "
               "GND-VCC. Conecta VCC a <b>3V3</b>, no a 5V/VIN."),
          P("2B.3 · Abrir el código y elegir el modelo", "h2"),
          *steps("Descomprime el proyecto. No cambies el nombre de la carpeta <b>ContadorYouTube</b>: Arduino "
                 "exige que se llame igual que el archivo <b>ContadorYouTube.ino</b> que hay dentro.",
                 "En Arduino IDE: <b>Archivo → Abrir…</b> y elige <b>ContadorYouTube.ino</b>.",
                 f"Solo para la pantalla táctil: abre la pestaña <b>config.h</b> y borra las dos barras del "
                 f"principio de la línea {code('// #define BOARD_CYD')}, para que quede "
                 f"{code('#define BOARD_CYD')}. Guarda con Ctrl+S. Para la OLED, déjala como está.", start=5),
          P("2B.4 · Elegir la placa y cargar", "h2"),
          *steps("Conecta la placa por USB.",
                 "En <b>Herramientas → Placa → esp32</b>, elige <b>ESP32 Dev Module</b>.",
                 "En <b>Herramientas → Partition Scheme</b>, elige <b>Minimal SPIFFS (1.9MB APP with OTA/190KB "
                 "SPIFFS)</b>. Es <b>imprescindible</b>: deja sitio para actualizarlo después sin cable.",
                 "En <b>Herramientas → Puerto</b>, elige el COM de la placa.",
                 "Pulsa <b>Subir</b> (el botón con una flecha). La primera vez compila 1-3 minutos; debe decir "
                 "«El Sketch usa … bytes (84 %)» con la táctil o (68 %) con la OLED, y al final «Hard resetting "
                 "via RTS pin».",
                 "La pantalla muestra <b>Configuración</b>: sigue en el paso 3.1.", start=8),
          table([
              ["Ajuste de Herramientas", "Valor"],
              ["Placa", "ESP32 Dev Module"],
              ["Partition Scheme", "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"],
              ["Puerto", "El COM de la placa (p. ej. COM5)"],
              ["El resto", "Sin cambiar"],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          tip("Si la carga se queda en «Connecting…», mantén pulsado <b>BOOT</b> hasta que avance.",
              "El <b>Monitor Serie</b> (lupa de arriba a la derecha, <b>115200 baudios</b>) muestra lo que hace: "
              "«YouTube: 0» en cada consulta correcta."),
          photo("Arduino IDE con «Hard resetting via RTS pin».", "La pantalla mostrando «Configuración».")]

# Paso 3
story += step_title(3, "Otras formas de configurarlo: móvil o tarjeta microSD")
story += [P("Si lo instalaste con el instalador de Windows, ya está configurado: salta al paso 4 o 5. Estas "
            "formas sirven tras instalarlo con Arduino IDE, o para cambiar de WiFi más adelante."),
          P("3.1 · Desde el móvil o el ordenador", "h2"),
          side("img/cyd-0_configuracion.png", 7 * cm, [
              *steps("Conéctate a la red WiFi abierta <b>YT-Dashboard</b>. En la pantalla táctil basta con "
                     "<b>escanear el código QR</b> con la cámara del móvil.",
                     "Se abre sola la página de configuración. Si no, entra en <b>192.168.4.1</b>.",
                     "Elige el idioma (<b>Idioma · Language</b>), tu WiFi, su contraseña, tu canal (@nombre) y "
                     "pega tu clave del paso 1.",
                     "Pulsa <b>Guardar</b>. El contador se conecta y en unos segundos muestra tus datos."),
          ]),
          warn("Escribe <b>siempre</b> la contraseña de la WiFi: la página no la recuerda.",
               "Si el móvil dice que «la red no tiene Internet», responde que quieres seguir conectado."),
          P(f"3.2 · Con la tarjeta microSD {TACTIL}", "h2"),
          P("Útil para regalarlo ya preparado o configurarlo sin móvil:"),
          *steps("En el ordenador, crea en la raíz de la tarjeta un archivo de texto llamado <b>config.txt</b> "
                 "(el proyecto trae uno de ejemplo en <b>tarjeta-sd/config.txt.ejemplo</b>) con:"),
          codeblock("wifi=NombreDeTuWiFi\ncontrasena=ContraseñaDeTuWiFi\ncanal=@tucanal\nclave_api=AIza...\n"
                    "idioma=es        (o en, para inglés)"),
          *steps("Mete la tarjeta en el contador y enciéndelo. Muestra «Configuración desde la tarjeta» con lo "
                 "que ha aplicado.",
                 "El archivo pasa a llamarse <b>config_aplicada.txt</b> y la contraseña queda tapada, para que "
                 "no se aplique en cada arranque ni quede escrita.", start=2),
          P("3.3 · Cambiar la configuración más adelante", "h2"),
          table([
              ["Qué cambiar", "Cómo"],
              ["El canal, la clave, el idioma, el brillo, los avisos…", "En su página de ajustes (paso 8), sin tocar "
               "el aparato."],
              ["La red WiFi", "Mantén <b>BOOT</b> 3 segundos y repite el apartado 3.1; o en el instalador, "
                              "<b>Solo cambiar la configuración</b>; o con config.txt."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm])]

# Paso 4 · OLED
story += step_title(4, "Uso diario del contador OLED")
story += [grid(["img/oled-1_suscriptores.png", "img/oled-2_vistas.png", "img/oled-3_video.png",
                "img/oled-4_grafico.png"], 7.6 * cm),
          table([
              ["Acción", "Qué hace"],
              ["Pulsación corta en <b>BOOT</b>", "Pasa a la siguiente vista. Tras la última aparece <b>AJUSTES</b>, "
                                                "con la dirección de la página de ajustes."],
              ["Mantener <b>BOOT</b> 3 segundos", "Abre la configuración (para cambiar de WiFi)."],
              ["Desenchufar y enchufar", "Se reinicia y vuelve a conectarse solo, sin perder nada."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          *bullets("Los suscriptores se actualizan cada 2 minutos y el último vídeo cada 10.",
                   "La vista <b>EL QUE MÁS CRECE</b> muestra cuál de tus 10 últimos vídeos gana más vistas en "
                   "24 h, cuántas, y cuántas veces más que la media de los demás. Aparece a la hora de "
                   "encenderlo (hasta entonces compara con menos horas).",
                   "Un <b>!</b> en la franja amarilla indica que la última consulta falló (sin Internet o cuota "
                   "agotada). Se recupera solo.",
                   "De 0:00 a 7:00 baja el brillo al mínimo (se puede desactivar en los ajustes).",
                   "Cada minuto desplaza el contenido 1 píxel para que la pantalla OLED no se «queme».")]

# Paso 5 · táctil
story += [PageBreak()]
story += step_title(5, "Uso diario de la pantalla táctil")
story += [P("Arriba se ve el nombre de tu canal y la hora (en <b>ámbar</b>, con un punto, si los datos no se "
            "han podido actualizar). Abajo, unos puntos indican en qué vista estás. Cambia de vista sola cada 6 "
            "segundos."),
          table([
              ["Acción", "Qué hace"],
              ["Tocar una zona vacía, o BOOT", "Pasa a la siguiente vista. Tras la última aparece <b>Ajustes</b>, "
                                              "con un QR que abre la página de ajustes."],
              ["Tocar algo con borde rojo y «›»", "Abre más detalles (tabla de abajo)."],
              ["Mantener <b>BOOT</b> 3 segundos", "Abre la configuración (para cambiar de WiFi)."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          P("5.1 · Lo que se puede tocar", "h2"),
          P("Todo lo que se puede tocar tiene el mismo aspecto: un recuadro con <b>borde rojo y «›»</b>. Mientras "
            "no lo hayas usado nunca, la vista lleva debajo una pista («Toca el recuadro: últimos 30 días») y el "
            "borde «late» al aparecer; en cuanto lo usas, la pista desaparece para siempre."),
          table([
              ["Dónde tocas", "Qué aparece"],
              ["El recuadro <b>comentarios</b> del último vídeo", "Los comentarios más recientes, de 2 en 2 "
                                                                "(toca para ver más)."],
              ["El <b>título</b> del último vídeo o de «el que más crece»", "Tus 10 últimos vídeos, de 4 en 4: "
               "vistas, las ganadas en 24 h (en verde) e interacción. Toca arriba a la derecha para ordenarlos "
               "por recientes o por los que más crecen."],
              ["Una <b>barra</b> del gráfico, o «+… hoy ›»", "Las vistas de ese día y su comparación con la media. "
                                                             "Toca otras barras para cambiar de día."],
              ["«+… en 7 días ›» de suscriptores", "La evolución de los suscriptores en los últimos 30 días."],
          ], [6 * cm, CONTENT_W - 6 * cm]),
          Spacer(1, 6),
          grid(["img/cyd-d1_comentarios.png", "img/cyd-d2_videos.png", "img/cyd-d3_dia.png",
                "img/cyd-d4_suscriptores.png"], 7.4 * cm,
               captions=["Comentarios (los nuevos, con punto rojo)", "Tus 10 últimos vídeos", "Vistas de un día",
                         "Suscriptores en 30 días"]),
          tip("Para volver, toca «‹» arriba o cualquier zona vacía. Si no tocas nada, vuelve solo a los 30 s.",
              "Los comentarios se piden a YouTube al tocarlos (1 unidad de cuota). Los vídeos ya están: se "
              "consultan solos cada 30 minutos.")]
story += [P("5.2 · El que más crece", "h2"),
          side("img/cyd-3b_tendencia.png", 7 * cm, [
              P("Compara tus <b>10 últimos vídeos</b> y muestra el que más vistas ha ganado en las últimas 24 "
                "horas: cuántas (en verde), cuántas veces más que la media de los demás («x3,2 tu media») y su "
                "<b>interacción</b> (me gusta y comentarios por cada 100 vistas)."),
              P("Tras encenderlo, la comparación empieza a la hora y va cubriendo más horas hasta llegar a 24. "
                "Toca el título para ver los 10 vídeos ordenados por lo que más crecen.", "body"),
          ]),
          grid(["img/cyd-d2_videos.png", "img/cyd-d2b_videos_crecen.png"], 7.4 * cm,
               captions=["Tus vídeos, los más recientes primero", "… o los que más crecen primero"])]

# Paso 6 · avisos
story += step_title(6, "Avisos, efectos y luz de ambiente", "")
story += [P(f"{TACTIL} &nbsp; Tras cada consulta, el contador compara con la anterior y avisa de lo más importante:"),
          table([
              ["Evento", "Pantalla", "LED", "Sonido"],
              ["Hito (100, 1.000, 10.000… suscriptores; nueva primera cifra de vistas)", "Tarjeta con confeti",
               "Arcoíris", "Fanfarria"],
              ["Suben los suscriptores", "La cifra sube contando y «+N» flota", "3 destellos verdes", "3 notas"],
              ["Comentario nuevo en el último vídeo", "Aviso azul con autor y texto (tócalo para leer)",
               "Doble pulso azul hasta que los ves", "«Ding»"],
              ["Vídeo nuevo", "Aviso rojo con el título", "3 destellos rojos", "3 notas"],
          ], [5 * cm, 5 * cm, 3.4 * cm, CONTENT_W - 13.4 * cm]),
          Spacer(1, 6),
          grid(["img/cyd-d8_hito.png", "img/cyd-d9_subida.png", "img/cyd-e1_aviso_comentario.png",
                "img/cyd-d6_comentarios_nuevos.png"], 7.4 * cm,
               captions=["Hito con confeti (en movimiento)", "Los suscriptores suben contando",
                         "Aviso de comentario nuevo", "Comentarios sin leer"]),
          *bullets("Para verlos sin esperar: en la página de ajustes, botones <b>Hito</b>, <b>Suscriptor</b>, "
                   "<b>Comentario</b> y <b>Vídeo</b> (usan los datos de tu canal).",
                   "El <b>sonido</b> necesita un altavoz en el conector «Speaker». Volumen en los ajustes (0 = sin "
                   "sonido).",
                   "Con «De noche» activado, de 0:00 a 7:00 no hay sonido ni LED."),
          P("6.1 · Luz de ambiente", "h2"),
          P("Cuando no hay avisos, el LED de la parte de atrás da ambiente. Se elige en los ajustes y se ve al "
            "momento mientras lo cambias:"),
          table([
              ["Ajuste", "Opciones"],
              ["Efecto", "Apagada · Fija · Respirando · Arcoíris lento · Según el canal (verde si hoy suben las "
                         "vistas, ámbar si no)"],
              ["Color", "Rojo YouTube, blanco cálido, azul, verde, morado, naranja, rosa, cian… o cualquiera"],
              ["Intensidad y velocidad", "5-100 % · lenta, normal o rápida"],
              ["Parpadeo sin conexión", "Un parpadeo ámbar cada 10 s si se queda sin Internet (desactivable)"],
          ], [4.5 * cm, CONTENT_W - 4.5 * cm]),
          Spacer(1, 6),
          tip("Si los colores no salen como en la página, ajusta el <b>balance de blancos</b> (Luz de ambiente → "
              "«Ajustar los colores del LED»): mueve las barras hasta que el LED se vea blanco puro. Los valores "
              "de fábrica (192/255/82) se midieron con una cámara en esta placa.")]

# Paso 7 · SD y batería
story += step_title(7, "Tarjeta microSD, sonidos y batería")
story += [P(f"{TACTIL} &nbsp; La tarjeta es opcional: sin ella todo funciona igual. Se lee al encender, así que "
            "métela con el contador desenchufado."),
          P("7.1 · Preparar la tarjeta (FAT32)", "h2"),
          P("El contador solo lee tarjetas en <b>FAT32</b>. Las de 32 GB o menos ya vienen así. Las de 64 GB o más "
            "vienen en exFAT y Windows no ofrece FAT32 para ellas; se crea una parte de 32 GB así:"),
          *steps("Mete la tarjeta en el ordenador y copia lo que quieras conservar: <b>esto la borra</b>.",
                 "Botón derecho en el botón de Inicio → <b>Administración de discos</b>.",
                 "Busca la tarjeta (el disco «Extraíble» del tamaño de la tarjeta; fíjate bien en que no sea tu "
                 "disco). Botón derecho sobre su partición → <b>Eliminar volumen</b> → Sí.",
                 "Botón derecho sobre «No asignado» → <b>Nuevo volumen simple</b> → tamaño <b>32000</b> MB → "
                 "sistema de archivos <b>FAT32</b> → Finalizar."),
          P("7.2 · Lo que hace con la tarjeta", "h2"),
          table([
              ["Archivo", "Para qué"],
              ["historial.csv", "Lo escribe él: una línea por consulta (fecha, suscriptores, vistas y datos del "
                                "último vídeo). Se abre con Excel. Se descarga desde la página de ajustes."],
              ["config.txt", "Configurarlo sin móvil (paso 3.2)."],
              ["sonidos/hito.wav, suscriptor.wav, comentario.wav, video.wav", "Sonidos propios para cada aviso, en "
               "vez de los de serie."],
              ["firmware.bin", "Actualizar el programa (paso 9)."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          tip("Para los sonidos: WAV mono de 16-22 kHz y como mucho 15 s. Con Audacity: abre tu sonido, "
              "<b>Pistas → Mezclar → Mezclar estéreo en mono</b>, frecuencia 22050 Hz abajo a la izquierda y "
              "<b>Archivo → Exportar → Exportar como WAV</b> (16 bits). No se admite MP3."),
          P("7.3 · Batería", "h2"),
          P("El conector <b>BAT</b> admite una batería LiPo de 3,7 V (conector JST de 1,25 mm); la placa la carga "
            "cuando hay USB. La placa no puede saber sola si hay batería, así que hay que indicarlo: en la página "
            "de ajustes, marca <b>Tengo una batería conectada</b>. Entonces la cabecera enseña el nivel y, al 10 %, "
            "avisa de que la cargues."),
          grid(["img/cyd-e2_bateria.png", "img/cyd-e3_bateria_baja.png"], 7.4 * cm,
               captions=["Nivel de batería en la cabecera", "Batería baja (en rojo)"]),
          warn("Con la pantalla siempre encendida, una batería pequeña dura <b>horas</b>, no días: es para "
               "moverlo un rato, no para dejarlo sin cable.",
               "Respeta la polaridad del conector (rojo = +). Una batería al revés puede estropear la placa.")]

# Paso 8 · ajustes web
story += [PageBreak()]
story += step_title(8, "Ajustes desde el navegador")
story += [P("El contador tiene su propia página de ajustes en tu red. No hace falta volver a la configuración "
            "inicial ni escribir la contraseña de la WiFi."),
          *steps("Con el móvil o el ordenador en <b>la misma WiFi</b> que el contador, abre en el navegador "
                 "<b>contador-xxxx.local</b> (lo dio el instalador). También lo enseña la vista <b>Ajustes</b> "
                 "del contador, con su dirección numérica; en la pantalla táctil, con un QR.",
                 "Cambia lo que quieras. El brillo, el volumen y la luz se ven al momento mientras los mueves.",
                 "Pulsa <b>Guardar</b>. Se aplica sin reiniciar y se conserva aunque lo desenchufes."),
          grid(["img/ajustes-web-1.png", "img/ajustes-web-2.png"], 6.6 * cm),
          P("La página de ajustes de la pantalla táctil, de arriba abajo (la del OLED no tiene las partes de "
            "avisos, luz y tarjeta).", "caption"),
          table([
              ["Ajuste", "Valores"],
              ["Idioma · Language", "Español o English: la pantalla y la página cambian al guardar"],
              ["Brillo · Cambiar de vista cada", "5-100 % · 3-60 s o «No cambiar»"],
              ["De noche", "De 0:00 a 7:00, brillo mínimo y (táctil) sin avisos"],
              ["Volumen · Destellos del LED", "0-100 % (0 = sin sonido) · sí/no"],
              ["Batería", "Marca si hay batería conectada"],
              ["Probar un aviso", "Hito, Suscriptor, Comentario, Vídeo"],
              ["Luz de ambiente", "Paso 6.1, con el balance de blancos"],
              ["Tarjeta microSD", "Espacio libre y descarga del historial"],
              ["Canal · Consultar cada · Clave", "@nombre o enlace · 1-1440 min (2 de fábrica) · vacía = mantener "
               "la actual"],
              ["Actualizar firmware", "Enlace al pie (paso 9)"],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          tip("Si <b>contador-xxxx.local</b> no abre (pasa en algunos Android), usa la dirección numérica de la "
              "vista Ajustes del contador.",
              "Si el navegador avisa «La conexión no es segura» o «Secure Site Not Available», pulsa "
              "<b>Continuar al sitio HTTP</b>: la página está en tu propia red.",
              "La página nunca muestra la clave completa ni la contraseña de la WiFi.")]

# Paso 9 · actualizar
story += step_title(9, "Actualizar el firmware", "2 min")
story += [P("Cuando salga una versión nueva, se instala sin cable y sin perder la configuración. La versión "
            "instalada aparece al pie de la página de ajustes (por ejemplo «Firmware 1.6.0 (cyd)»)."),
          P("Desde la página de ajustes (los dos modelos)", "h2"),
          *steps("Descarga el archivo de tu modelo: <b>contador-cyd-v….bin</b> (pantalla táctil) o "
                 "<b>contador-oled-v….bin</b> (OLED). No uses el que termina en «-completo».",
                 "En la página de ajustes, abajo, pulsa <b>Actualizar firmware</b>.",
                 "Elige el archivo y pulsa <b>Instalar</b>. El contador muestra una barra de progreso, se "
                 "reinicia en unos 15 segundos y la página vuelve a los ajustes."),
          P(f"Con la tarjeta microSD {TACTIL}", "h2"),
          *steps("Copia el archivo a la raíz de la tarjeta y cámbiale el nombre a <b>firmware.bin</b>.",
                 "Mete la tarjeta y enciende el contador: instala con barra de progreso y se reinicia. El archivo "
                 "pasa a llamarse firmware_instalado.bin."),
          tip("Es seguro: el firmware nuevo se escribe aparte y solo se activa si llegó entero y es del mismo "
              "modelo. Si eliges el de la otra placa o se va la luz, sigue funcionando el anterior.",
              "El instalador de Windows también sirve, pero <b>Reinstalar</b> borra la configuración y el "
              "historial guardado en el aparato (el de la tarjeta no)."),
          ]

# Paso 10 · carcasa
story += step_title(10, "Colocarlo en el escritorio (opcional)")
story += [*bullets("Pantalla táctil: hay muchas carcasas imprimibles en 3D para la «ESP32-2432S028» o «Cheap "
                   "Yellow Display» de 2,8\". Deja acceso al USB-C, al botón BOOT y a la ranura de la tarjeta.",
                   "OLED: carcasas de «ESP32 + OLED 0,96\"», o una cajita de cartón o madera con la pantalla "
                   "pegada con cinta de doble cara.",
                   "Esconde el cable por detrás del monitor."),
          photo("El contador terminado sobre el escritorio (foto de portada).",
                "Un primer plano tocando la pantalla y el aviso de comentario.")]

# Lista de pruebas
story += [PageBreak(), P("Lista de pruebas", "h1"),
          P("Marca cada prueba al comprobarla. Las marcadas con (T) son solo de la pantalla táctil.")]
tests = [
    ("Instalador", "Seguir el paso 2", "Termina en «Tu contador ya está funcionando»"),
    ("Datos correctos", "Comparar con tu canal en YouTube", "Suscriptores y vistas coinciden (redondeados)"),
    ("Rotación", "Observar 30 segundos", "Cambia de vista cada 6 segundos"),
    ("Botón corto", "Pulsar BOOT una vez", "Pasa a la siguiente vista"),
    ("Botón largo", "Mantener BOOT 3 segundos", "Aparece la configuración y la red YT-Dashboard"),
    ("Reinicio", "Desenchufar y volver a enchufar", "Se conecta solo, sin pedir nada"),
    ("Sin Internet", "Apagar el router 10 minutos", "Se marca que no está al día y se recupera solo"),
    ("Ajustes", "Abrir contador-xxxx.local y mover el brillo", "La pantalla cambia al momento"),
    ("Idioma", "Elegir English en los ajustes y guardar", "La pantalla y la página pasan a inglés"),
    ("El que más crece", "Mirar esa vista una hora después de encenderlo", "Sale el vídeo que más vistas gana y cuántas"),
    ("(T) Toques", "Tocar comentarios, título, una barra y «+… en 7 días»", "Se abren los cuatro detalles"),
    ("(T) Pistas", "Usar una zona", "Su pista desaparece y no vuelve"),
    ("(T) Avisos", "Botones de prueba en los ajustes", "Confeti, cifra contando y avisos que bajan"),
    ("(T) Luz", "Cambiar efecto, color e intensidad", "El LED cambia al momento; blanco sin tinte"),
    ("(T) Tarjeta", "Meter una microSD en FAT32 y esperar 5 min", "Aparece en los ajustes y descarga el historial"),
    ("Actualizar", "Subir el .bin en Actualizar firmware", "Se reinicia con la versión nueva"),
    ("A los 1-2 días", "Mirar las vistas, el gráfico y los suscriptores",
     "Aparecen «+… hoy», el gráfico y el cambio en días"),
    ("Modo noche", "Mirar la pantalla después de las 0:00", "El brillo es más bajo"),
]
rows = [["", "Prueba", "Cómo", "Resultado esperado", "Notas"]]
rows += [[CheckBox(), f"<b>{n}</b>", how, exp, ""] for n, how, exp in tests]
story += [table(rows, [0.8 * cm, 3.1 * cm, 4.6 * cm, 5.2 * cm, CONTENT_W - 13.7 * cm]), PageBreak()]

# Solución de problemas
story += [P("Solución de problemas", "h1"),
          table([
              ["Síntoma", "Causa probable", "Solución"],
              ["«Windows protegió su PC» al abrir el instalador", "El instalador no está firmado",
               "Más información → Ejecutar de todas formas (solo si lo bajaste de la página oficial)"],
              ["El instalador o el ordenador no detectan la placa", "Cable de solo carga o falta el controlador CH340",
               "Otro cable USB-C; instala el CH340 con el botón del instalador"],
              ["«Comprobar» dice que la clave no vale", "Mal copiada, API sin habilitar o recién creada",
               "Repasa el paso 1; espera 5 minutos tras crearla"],
              ["No se conecta a mi WiFi", "Red de 5 GHz o contraseña incorrecta",
               "Usa la red de 2,4 GHz del router (suele llamarse igual sin «5G» o «PLUS»)"],
              ["Tarda en conectar («Intento 2 de 12»)", "Algunos routers rechazan un rato la reconexión",
               "Es normal: suele conectar al segundo o tercer intento"],
              ["«Canal no encontrado»", "Nombre mal escrito", "Usa el @nombre exacto de la dirección del canal"],
              ["«Clave API no válida» en la pantalla", "Clave borrada, sin habilitar o mal pegada",
               "Pon una nueva en la página de ajustes"],
              ["La hora sale en ámbar (o «!» en la OLED)", "Sin Internet o cuota agotada",
               "Se recupera solo; la cuota se renueva a las 9:00 (hora de España)"],
              ["No abre contador-xxxx.local", "El móvil no resuelve nombres .local o está en otra red",
               "Usa la dirección numérica de la vista Ajustes; misma WiFi"],
              ["(T) Los toques no caen donde toco", "Calibración del táctil distinta en tu placa",
               "Ver «Calibrar el táctil» en el README del proyecto"],
              ["(T) No aparece la tarjeta", "No está en FAT32 o se metió con el aparato encendido",
               "Paso 7.1; métela desenchufado"],
              ["(T) Los colores del LED no cuadran", "Cada LED luce distinto",
               "Ajusta el balance de blancos (paso 6.1)"],
              ["(T) No suena", "No hay altavoz o el volumen está a 0", "Conecta un altavoz de 8 Ω; sube el volumen"],
              ["«No se ha instalado: este archivo no es…»", "Firmware de la otra placa",
               "Descarga el .bin de tu modelo (cyd = táctil, oled = OLED)"],
              ["Arduino IDE: «Sketch too big»", "Esquema de particiones por defecto",
               "Partition Scheme → Minimal SPIFFS (paso 2B.4)"],
              ["Arduino IDE: «LovyanGFX.hpp / U8g2lib.h: No such file», o la táctil en negro",
               "Falta la librería de tu modelo, o activar BOARD_CYD", "Pasos 2B.1 y 2B.3"],
          ], [4.8 * cm, 5 * cm, CONTENT_W - 9.8 * cm]),
          PageBreak()]

# Cómo funciona
story += [P("Cómo funciona por dentro", "h1"),
          *steps("Al encenderse lee su configuración (WiFi, canal, clave, ajustes) de la memoria interna. Durante "
                 "un segundo escucha por el cable USB por si el instalador le envía una nueva.",
                 "Si no tiene configuración, crea la red YT-Dashboard con un portal cautivo (WiFiManager).",
                 "Se conecta a la WiFi y sincroniza la hora por Internet (NTP).",
                 "Consulta la YouTube Data API v3 por HTTPS, verificando el certificado de Google.",
                 "Guarda un registro diario para calcular las vistas por día y el crecimiento, y compara cada "
                 "consulta con la anterior para los avisos.",
                 "Dibuja las vistas (U8g2 en la OLED, LovyanGFX en la táctil) y repite cada pocos minutos."),
          P("Consumo de cuota", "h2"),
          table([
              ["Consulta", "Frecuencia", "Unidades al día"],
              ["Datos del canal", "Cada 2 minutos (de fábrica)", "720"],
              ["Último vídeo (2 consultas)", "Cada 10 minutos", "288"],
              ["Tus 10 últimos vídeos (2 consultas)", "Cada 30 minutos", "96"],
              ["Comentarios", "Solo al tocarlos (táctil)", "1 cada vez"],
              ["<b>Total por contador</b>", "", "<b>≈ 1.100</b> (límite gratuito: 10.000 por clave)"],
          ], [6 * cm, 4.5 * cm, CONTENT_W - 10.5 * cm]),
          Spacer(1, 6),
          P("Privacidad", "h2"),
          P("Solo se consultan datos públicos del canal; no se inicia sesión con ninguna cuenta. La clave y la "
            "contraseña de la WiFi se guardan únicamente en el aparato; el instalador las envía por el cable USB."),
          P("Estructura del proyecto", "h2"),
          codeblock("ContadorYouTube/           el sketch que se abre en Arduino IDE\n"
                    "  ContadorYouTube.ino      programa principal (OLED y táctil)\n"
                    "  config.h                 qué modelo se compila\n"
                    "  src/settings, portal     configuración y portal cautivo\n"
                    "  src/provision            configuración por USB y config.txt\n"
                    "  src/youtube, certs.h     consultas HTTPS a YouTube\n"
                    "  src/history              registro diario\n"
                    "  src/oled/, src/cyd/      dibujo de cada pantalla\n"
                    "  src/alerts               LED y sonido (táctil)\n"
                    "  src/sdcard, battery      tarjeta y batería (táctil)\n"
                    "  src/webui, updater       página de ajustes y actualización\n"
                    "tools/instalador/          instalador de Windows\n"
                    "tools/                     vistas previas, este manual y build_firmware.py"),
          PageBreak()]

# Instructable
story += [P("Notas para el Instructable", "h1"),
          table([
              ["Elemento", "Propuesta"],
              ["Título", "Contador de YouTube táctil por 17 € (ESP32 «Cheap Yellow Display»)"],
              ["Introducción", "Qué hace, cuánto cuesta, que no hay que soldar y un GIF con los avisos y el "
                               "confeti."],
              ["Pasos", "Material → Clave de la API → Instalador (o Arduino IDE) → Uso y toques → Avisos y luz → "
                        "Tarjeta y batería → Carcasa"],
              ["Archivos", "Enlace al repositorio y al instalador; el STL de la carcasa si la haces."],
              ["Etiquetas", "ESP32, CYD, YouTube, Arduino, IoT, pantalla táctil, gadget de escritorio"],
          ], [3.5 * cm, CONTENT_W - 3.5 * cm]),
          Spacer(1, 6),
          P("Antes de publicar", "h2"),
          *bullets("Comprueba que <b>secrets.h no está</b> en el repositorio ni en los archivos publicados.",
                   "Graba un vídeo corto: tocar los comentarios, la cifra subiendo y el confeti.",
                   "Monta un segundo contador siguiendo solo este manual, para asegurarte de que no falta nada.",
                   "Deja claro que cada lector crea su propia clave de API, gratis.")]

# Anexo A
story += [PageBreak(), P("ANEXO A", "kicker"), P("Publicar el firmware y el instalador", "h1"),
          P("Para quien distribuye el contador. El firmware publicado <b>nunca lleva una clave de API</b>: cada "
            "usuario pone la suya. La clave de <b>secrets.h</b> es solo para tus pruebas."),
          *steps(f"Sube el número de versión en {code('src/version.h')} ({code('FIRMWARE_VERSION')}).",
                 f"Ejecuta {code('python tools/build_firmware.py')}. Compila las dos placas sin la clave "
                 "(PUBLIC_BUILD), comprueba que no se ha colado y deja en <b>dist/</b>, por placa, el .bin de "
                 "actualización y el -completo.bin (instalación desde cero).",
                 f"Ejecuta {code('python tools/instalador/build_exe.py')}: crea "
                 "<b>dist/InstaladorContadorYouTube.exe</b> con el firmware nuevo dentro (unos 18 MB).",
                 "Publica el instalador y los dos .bin de actualización."),
          tip("Para que Windows no muestre «Windows protegió su PC», el instalador tiene que ir firmado con un "
              "certificado de firma de código (de pago, unos cientos de euros al año).",
              "Cada placa lleva escrito su modelo en el firmware: la actualización rechaza el de la otra.")]
story += [P("Licencias", "h2"),
          table([["Parte", "Licencia", "Archivo"],
                 ["Código y firmware", "PolyForm Noncommercial 1.0.0: uso, cambios y copias sin fines "
                  "comerciales; para vender, permiso del autor", "LICENSE.md"],
                 ["Instalador de Windows", "GPL-2.0-or-later (lleva dentro esptool, que es GPL)",
                  "tools/instalador/LICENSE"],
                 ["Este manual", "CC BY-NC 4.0: copiar y adaptar citando al autor, sin fines comerciales",
                  "LICENCIA-MANUAL.txt"],
                 ["Bibliotecas y tipografías", "Las de cada autor (MIT, BSD, LGPL, OFL…)", "AVISOS-TERCEROS.txt"]],
                [3.4 * cm, 8.6 * cm, 5 * cm]),
          P("Quien comparta el código o el firmware debe incluir esos archivos (o enlazarlos) y el aviso "
            "«Required Notice: Copyright (c) 2026 Adrian Cubas».", "small")]

# Anexo B
story += [CondPageBreak(12 * cm), P("ANEXO B", "kicker"), P("Versión con pantalla de tinta electrónica", "h1"),
          P("El mismo proyecto incluye una tercera versión, experimental, para la <b>LilyGo T5 4.7\" ESP32-S3</b> "
            "(960×540, 16 grises, unos 40 €). Funciona con batería durante semanas porque duerme entre "
            "actualizaciones y la tinta conserva la imagen sin gastar energía."),
          image("preview-dashboard.png", 13 * cm, "Vista previa aproximada del panel de tinta."),
          *bullets("Compra la versión <b>ESP32-S3 normal</b>, no la «Pro» (es otro hardware).",
                   f"Se carga con PlatformIO (VS Code): {code('pio run -e t5-epd47-s3 -t upload')}.",
                   "No tiene instalador, ni avisos, ni actualización sin cable.")]

# ------------------------------------------------------------------- PDF ---

doc = SimpleDocTemplate(str(OUT), pagesize=A4, leftMargin=MARGIN, rightMargin=MARGIN, topMargin=MARGIN,
                        bottomMargin=2.2 * cm, title="Contador de YouTube de escritorio",
                        author="Adrian Cubas", subject="Manual de montaje y uso")
doc.build(story, onFirstPage=on_page, onLaterPages=on_page)
print(OUT)
