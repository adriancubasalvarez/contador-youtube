"""Builds the English PDF manual of the YouTube counter (CYD touch screen and OLED).

    python tools/manual/build_manual_en.py docs/manual-youtube-counter.pdf

Same layout as the Spanish one (build_manual.py, styles in manual_common.py),
with the English screenshots of docs/img-en/: CYD and OLED screens
(cyd_preview / oled_preview with "en"), installer (installer_screens.py ... en),
settings page, wiring and Google Cloud (labels in English).
"""
import sys

from manual_common import *  # noqa: F401,F403  styles and helpers
import manual_common
from reportlab.lib.units import cm
from reportlab.platypus import CondPageBreak, PageBreak, SimpleDocTemplate, Spacer, Table, TableStyle

OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else DOCS / "manual-youtube-counter.pdf"
manual_common.TEXT.update({"tip": "Tip", "warn": "Warning", "photo": "For the Instructable: photos for this step",
                           "step": "STEP", "footer": "Desktop YouTube counter · build and user manual"})

TOUCH = badge("TOUCH SCREEN")
I = "img-en/"  # English screenshots

story = []

# Cover
story += [
    Spacer(1, 1.0 * cm),
    P("BUILD AND USER MANUAL", "kicker"),
    Spacer(1, 6),
    P("Desktop YouTube counter", "cover_title"),
    Spacer(1, 8),
    P("A small ESP32 gadget that shows your channel's subscribers, views, latest video and comments in real "
      "time. Two models: color touch screen or OLED screen. In English or Spanish.", "cover_sub"),
    Spacer(1, 0.8 * cm),
    grid([I + "cyd-1_suscriptores.png", I + "oled-1_suscriptores.png"], 7.6 * cm,
         captions=["Color touch screen (2.8\")", "OLED counter (0.96\")"]),
    Spacer(1, 0.6 * cm),
]
facts = Table([[P("<b>≈ €17 / $20</b><br/>touch · ≈ €12 OLED", "cell"), P("<b>≈ 15 min</b><br/>with the installer", "cell"),
                P("<b>No soldering</b><br/>no coding", "cell"), P("<b>Updates</b><br/>over WiFi", "cell")]],
              colWidths=[CONTENT_W / 4] * 4)
facts.setStyle(TableStyle([
    ("LINEABOVE", (0, 0), (-1, 0), 1.2, INK), ("LINEBELOW", (0, 0), (-1, 0), 0.5, LINE),
    ("TOPPADDING", (0, 0), (-1, -1), 10), ("BOTTOMPADDING", (0, 0), (-1, -1), 10),
]))
story += [facts, Spacer(1, 1 * cm),
          P("Manual version 2.1 · October 2026 · firmware 1.6.0 · sketch <b>ContadorYouTube</b> · "
            "<i>Manual en español: manual-contador-youtube.pdf</i>", "small"),
          P("© 2026 Adrian Cubas. Manual licensed CC BY-NC 4.0; code and firmware licensed PolyForm "
            "Noncommercial 1.0.0: you may build it, modify it and share it for non-commercial purposes. To sell "
            "it, ask the author for permission. Details in appendix A.", "small"),
          PageBreak()]

# Contents
story += [P("Contents", "h1")]
toc = [
    ("What you will build", ""), ("Parts", ""),
    ("Step 1 · Create your YouTube API key", "10 min"),
    ("Step 2 · Install it with the Windows installer (recommended)", "5 min"),
    ("Step 2B · Alternative: install it with Arduino IDE", "30 min"),
    ("Step 3 · Other ways to set it up: phone or microSD card", ""),
    ("Step 4 · Everyday use of the OLED counter", ""),
    ("Step 5 · Everyday use of the touch screen", ""),
    ("Step 6 · Alerts, effects and ambient light (touch)", ""),
    ("Step 7 · microSD card, sounds and battery (touch)", ""),
    ("Step 8 · Settings from the browser", ""),
    ("Step 9 · Update the firmware", "2 min"),
    ("Step 10 · Put it on your desk (optional)", ""),
    ("Test checklist", ""), ("Troubleshooting", ""), ("How it works", ""),
    ("Notes for the Instructable", ""),
    ("Appendix A · Publishing the firmware and the installer", ""),
    ("Appendix B · E-ink version", ""),
]
story += [table([[t, d] for t, d in toc], [CONTENT_W - 3 * cm, 3 * cm], header=False),
          Spacer(1, 10),
          P(f"Sections marked {TOUCH} only apply to the touch screen model.", "small"),
          PageBreak()]

# What you will build
story += [P("What you will build", "h1"),
          P("A desk counter that joins your WiFi and checks your YouTube channel's <b>public</b> stats every few "
            "minutes. It runs on its own, plugged into any phone charger, and cycles through five views. There "
            "are two models running the same program:"),
          table([
              ["", "Color touch screen", "OLED counter"],
              ["Screen", "2.8\", 320×240, color, touch", "0.96\", 128×64, yellow and blue"],
              ["Assembly", "None: the board already has the screen", "4 wires, no soldering"],
              ["Approx. price", "≈ €17 / $20", "≈ €12 / $14"],
              ["On tap", "Comments, your 10 latest videos, views of a day, subscribers over 30 days", "—"],
              ["Alerts", "On-screen effects, color LED and sound", "—"],
              ["Extras", "microSD card (history, sounds, setup), battery", "—"],
              ["Language", "English or Spanish (screen, settings and alerts)", "English or Spanish"],
          ], [3 * cm, (CONTENT_W - 3 * cm) / 2, (CONTENT_W - 3 * cm) / 2]),
          Spacer(1, 10),
          grid([I + "cyd-1_suscriptores.png", I + "cyd-2_vistas.png", I + "cyd-3_video.png", I + "cyd-4_grafico.png"],
               5.4 * cm, captions=["Subscribers and their change over 7 days", "Total views and today's views",
                                   "Latest video: views, likes and comments", "Views per day (2 weeks)"]),
          P("Four of the five views of the touch screen (the fifth one, \"fastest growing\", is in step 5; the "
            "OLED counter shows the same in small). Images rendered with the same code the device runs; the "
            "data are examples.", "caption"),
          tip("The public YouTube API does not give views per day. The counter works them out by keeping a daily "
              "record, so the chart, \"today +…\" and the growth show up after 1-2 days of use.",
              "YouTube rounds subscribers to 3 significant digits (128,400, not 128,437): the number changes when "
              "the third digit changes."),
          PageBreak()]

# Parts
story += [P("Parts", "h1"),
          P("Color touch screen", "h2"),
          table([
              ["Part", "Details", "Approx. price"],
              ["2.8\" screen board", "Freenove \"ESP32 CYD 2.8\" … <b>ILI9341</b> TN Touch\" (FNK0114F), with the "
                                    "ESP32 built in. Other ILI9341 \"Cheap Yellow Display\" boards should work, "
                                    "but this is the tested one.", "€17 / $20"],
              ["USB-C <b>data</b> cable", "To install it and power it", "—"],
              ["5 V USB charger", "Any phone charger (1 A)", "—"],
          ], [4 * cm, CONTENT_W - 6.5 * cm, 2.5 * cm]),
          Spacer(1, 4),
          P("Optional for the touch screen", "h2"),
          table([
              ["Part", "What for"],
              ["microSD card", "Full history, your own sounds and setup without a phone. Ideally 32 GB or less "
                               "(they come in FAT32); larger ones need preparing (step 7)."],
              ["8 Ω speaker, 1-3 W", "Alert sounds. With a 2-pin 1.25 mm JST plug (the board's \"Speaker\" "
                                     "connector)."],
              ["3.7 V LiPo battery", "Use it without a cable for a while. 1.25 mm JST plug (\"BAT\" connector); the "
                                     "board charges it over USB."],
          ], [4 * cm, CONTENT_W - 4 * cm]),
          Spacer(1, 4),
          P("OLED counter", "h2"),
          table([
              ["Part", "Details", "Approx. price"],
              ["ESP32 board", "ESP32-WROOM-32 with USB-C and CH340 chip (e.g. a Diymore NodeMCU, sold in packs "
                              "of 2)", "€8 / $9"],
              ["0.96\" OLED screen", "128×64, SSD1306, 4-pin I2C, yellow and blue, with 4 female-female dupont "
                                    "wires (e.g. ELEGOO, sold in packs of 3)", "€4 / $5"],
          ], [4 * cm, CONTENT_W - 6.5 * cm, 2.5 * cm]),
          Spacer(1, 4),
          P("Also", "h2"),
          *bullets("A computer with <b>Windows 10 or 11</b> for the installer (or Windows, macOS or Linux with "
                   "Arduino IDE).",
                   "A Google account to create the API key (free, no card).",
                   "A <b>2.4 GHz</b> WiFi network: the ESP32 does not work with 5 GHz networks.",
                   "A USB-C <b>data</b> cable: many cheap cables are \"charge only\". If the computer does not "
                   "detect the board, try another cable first."),
          PageBreak()]

# Step 1
story += step_title(1, "Create your YouTube API key", "10 min")
W1 = CONTENT_W * 0.78
story += [P("The counter reads YouTube's public data through the <b>YouTube Data API v3</b>, which needs a free "
            "key. <b>Everyone creates their own</b> with their Google account: it costs nothing, needs no card "
            "and is done only once. Do it before installing, because step 2 asks for it."),
          P("The screenshots show the Google Cloud console in Spanish; the English one has exactly the same "
            "layout. The red labels say where to click.", "small"),
          P("1.1 · Create a project", "h2"),
          *steps("Go to <b>console.cloud.google.com</b> with your Google account and accept the terms if it is "
                 "your first time.",
                 "Top left, next to the Google Cloud logo, click the button with the current project name (the "
                 "first time it says \"Select a project\")."),
          image(I + "gcloud-selector-proyecto.png", W1),
          *steps("In the window that opens, click <b>New project</b>, top right.", start=3),
          image(I + "gc-01-selector.png", W1),
          *steps("Type the name <b>contador-youtube</b> (or any name). Leave \"Location\" as it is (<i>No "
                 "organization</i>) and click <b>Create</b>.", start=4),
          image(I + "gc-03-nombre.png", W1),
          *steps("Once created, <b>the console still shows the previous project</b>. Open the notice in the bell "
                 "icon (top right) and click <b>Select project</b>. Check that the button at the top now shows "
                 "your new project.", start=5),
          image(I + "gc-04-notificacion.png", CONTENT_W * 0.55),
          P("1.2 · Enable the YouTube Data API v3", "h2"),
          *steps("Open the main menu (the three lines, top left) → <b>APIs &amp; Services</b> → <b>Library</b>. "
                 "Search for <b>YouTube Data API v3</b> and open it.",
                 "Click <b>Enable</b> and wait a few seconds.", start=6),
          image(I + "gc-05-api-habilitar.png", W1),
          *steps("The API page opens. <b>Status</b> must say <b>Enabled</b>.", start=8),
          image(I + "gc-06-api-habilitada.png", CONTENT_W),
          P("1.3 · Create the key", "h2"),
          *steps("In the side menu click <b>Credentials</b> and then <b>+ Create credentials</b>. The yellow "
                 "notice about the \"OAuth consent screen\" does not affect this project: ignore it.", start=9),
          image(I + "gc-07-credenciales.png", CONTENT_W),
          *steps("Choose <b>API key</b>.", start=10),
          image(I + "gc-08-menu-clave.png", CONTENT_W * 0.6),
          *steps("The <b>Create API key</b> panel opens. Name it <b>contador-youtube</b>. Under <b>API "
                 "restrictions</b>, type \"YouTube Data\" in the filter, tick <b>YouTube Data API v3</b> and "
                 "click <b>OK</b>.", start=11),
          image(I + "gc-10-restriccion.png", CONTENT_W * 0.55),
          *steps("Check that it says <b>1 API</b> and that <b>Application restrictions</b> is set to "
                 "<b>None</b> (the counter has no fixed address). Click <b>Create</b>.", start=12),
          image(I + "gc-11-crear-clave.png", CONTENT_W * 0.45),
          *steps("<b>API key created</b> appears. Click the copy button to the right of the key (it starts with "
                 "<b>AIza</b>) and paste it into a notepad for step 2. Then click <b>Close</b>.", start=13),
          image(I + "gc-12-clave-creada.png", CONTENT_W * 0.45, "The key is hidden in this screenshot."),
          tip("No card is needed: the free tier includes 10,000 units a day and a counter uses about 1,100. If it "
              "were exceeded, the API stops answering until the next day, but it never charges.",
              "If you lose the key, you can see it again in <b>Credentials</b> → <b>API keys</b> → "
              "<b>Show key</b>."),
          warn("Treat the key like a password: do not share or publish it. After creating it, it may take a few "
               "minutes to work.",
               "Do not enable billing on this project and keep the key restricted to the YouTube Data API v3: that "
               "way it can never generate costs.")]

# Step 2
story += [PageBreak()]
story += step_title(2, "Install it with the Windows installer", "5 min")
IW = 12 * cm
story += [P("The installer does everything over the USB cable: it writes the program to the board and sends it "
            "your WiFi, your channel, your key and the language. No Arduino IDE and no phone needed."),
          *steps("Download <b>InstaladorContadorYouTube.exe</b> and double-click it. Nothing else to install: the "
                 "counter's program is inside.",
                 "If Windows shows <b>\"Windows protected your PC\"</b>, click <b>More info</b> → <b>Run "
                 "anyway</b>. It appears because the installer is not signed with a paid certificate yet. You can "
                 "check the file is the original with its SHA-256 fingerprint (see the README)."),
          warn("Only run the installer downloaded from the project's official page."),
          *steps("<b>Pick your model</b> (touch screen or OLED) and click <b>Next</b>. Top right you can switch the "
                 "installer language (<b>Español</b> or <b>English</b>); it starts in your Windows language.",
                 start=3),
          image(I + "instalador-1_placa.png", IW),
          *steps("<b>Connect the counter</b> to the computer with the USB cable. The window detects it and says "
                 "\"Board ready on COM…\". Click <b>Install the counter</b>.", start=4),
          image(I + "instalador-2_conectar.png", IW),
          tip("If it does not appear, try another cable or port. The first time, Windows may need the <b>CH340</b> "
              "driver: the installer shows a button to download it.",
              "If the counter already had the program, it offers <b>Only change the settings</b>, without "
              "reinstalling."),
          *steps("Wait for the bar to finish (under a minute). Do not unplug the cable.", start=5),
          image(I + "instalador-3_grabando.png", IW),
          *steps("<b>Fill in your details</b>: pick your WiFi network (it must be 2.4 GHz; if your computer is on "
                 "the 5 GHz one, the list suggests the other), its password, your channel (the YouTube @handle "
                 "or the link), the key from step 1 and the <b>counter language</b> (English or Spanish). Click "
                 "<b>Check and send</b>.", start=6),
          image(I + "instalador-4_configurar.png", IW),
          P("The installer checks YouTube from your computer before sending anything: if the key or the channel "
            "are wrong, it tells you what is going on (key badly copied, API not enabled, channel not found…)."),
          *steps("Watch the three steps get ticked: it sends the settings over the cable, the counter joins your "
                 "WiFi and checks YouTube.", start=7),
          image(I + "instalador-5_enviando.png", IW),
          *steps("<b>Done!</b> Note the address of its settings page (<b>contador-xxxx.local</b>) or click "
                 "<b>Create a desktop shortcut</b>. You can now unplug it from the computer and plug it into a "
                 "charger.", start=8),
          image(I + "instalador-6_listo.png", IW),
          tip("The name <b>contador-xxxx.local</b> never changes. The numeric address (192.168…) is handed out by "
              "the router and may change; if you want it fixed, reserve it in your router (\"static DHCP\" or "
              "\"IP reservation\") using the MAC address shown by the router.")]

# Step 2B
story += step_title("2B", "Alternative: install it with Arduino IDE", "30 min")
story += [P("For those who want to compile the code (or do not use Windows). When done, set it up with your "
            "phone (step 3)."),
          P("2B.1 · Prepare Arduino IDE", "h2"),
          *steps("Download <b>Arduino IDE 2</b> from <b>arduino.cc/en/software</b> and install it. Open it.",
                 "Open the <b>Boards Manager</b> (second icon of the left sidebar), search for <b>esp32</b> and "
                 "click <b>Install</b> on <b>esp32 by Espressif Systems</b>. It takes several minutes."),
          warn("Do not pick \"Arduino ESP32 Boards\": that one is only for Arduino Nano ESP32 boards."),
          *steps("Open the <b>Library Manager</b> (third icon, some books) and install these. If it asks about "
                 "dependencies, click <b>Install all</b>:", start=3),
          table([
              ["Search", "Install the one by", "Model", "What it does"],
              ["WiFiManager", "tzapu", "Both", "The YT-Dashboard network and the setup page"],
              ["ArduinoJson", "Benoit Blanchon", "Both", "Read YouTube's answers"],
              ["LovyanGFX", "lovyan03", "Touch", "Draw on the color screen"],
              ["U8g2", "oliver", "OLED", "Draw on the OLED screen"],
          ], [3 * cm, 3.4 * cm, 2 * cm, CONTENT_W - 8.4 * cm]),
          Spacer(1, 6),
          P("2B.2 · Wire the screen (OLED only)", "h2"),
          P("The touch screen comes assembled: skip this. For the OLED, with the board <b>unplugged from "
            "USB</b>, connect the 4 pins:"),
          image(I + "conexiones.png", CONTENT_W * 0.85),
          table([
              ["OLED pin", "ESP32 pin", "Suggested color"],
              ["GND", "GND (any)", "Black"],
              ["VCC", "3V3", "Red"],
              ["SCL", "GPIO 22 (labeled D22 or 22)", "Yellow"],
              ["SDA", "GPIO 21 (labeled D21 or 21)", "Blue"],
          ], [4 * cm, 7 * cm, CONTENT_W - 11 * cm]),
          Spacer(1, 6),
          warn("Read the names printed on your screen: on some models the order is VCC-GND instead of GND-VCC. "
               "Connect VCC to <b>3V3</b>, not to 5V/VIN."),
          P("2B.3 · Open the code and choose the model", "h2"),
          *steps("Unzip the project. Do not rename the <b>ContadorYouTube</b> folder: Arduino requires it to have "
                 "the same name as the <b>ContadorYouTube.ino</b> file inside.",
                 "In Arduino IDE: <b>File → Open…</b> and choose <b>ContadorYouTube.ino</b>.",
                 f"Touch screen only: open the <b>config.h</b> tab and delete the two slashes at the start of the "
                 f"line {code('// #define BOARD_CYD')}, so it reads {code('#define BOARD_CYD')}. Save with "
                 f"Ctrl+S. For the OLED, leave it as it is.", start=5),
          P("2B.4 · Choose the board and upload", "h2"),
          *steps("Connect the board over USB.",
                 "In <b>Tools → Board → esp32</b>, choose <b>ESP32 Dev Module</b>.",
                 "In <b>Tools → Partition Scheme</b>, choose <b>Minimal SPIFFS (1.9MB APP with OTA/190KB "
                 "SPIFFS)</b>. This is <b>essential</b>: it leaves room for later updates over WiFi.",
                 "In <b>Tools → Port</b>, choose the board's COM port.",
                 "Click <b>Upload</b> (the arrow button). The first compile takes 1-3 minutes; it should say "
                 "\"Sketch uses … bytes (85%)\" for the touch screen or (68%) for the OLED, and at the end "
                 "\"Hard resetting via RTS pin\".",
                 "The screen shows <b>Configuración</b> (setup): continue with step 3.1 and choose English there.",
                 start=8),
          table([
              ["Tools setting", "Value"],
              ["Board", "ESP32 Dev Module"],
              ["Partition Scheme", "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"],
              ["Port", "The board's COM port (e.g. COM5)"],
              ["Everything else", "Unchanged"],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          tip("If the upload hangs on \"Connecting…\", hold <b>BOOT</b> until it moves on.",
              "The <b>Serial Monitor</b> (magnifier, top right, <b>115200 baud</b>) shows what it does: "
              "\"YouTube: 0\" on every successful check (the log is in Spanish)."),
          photo("Arduino IDE showing \"Hard resetting via RTS pin\".", "The screen showing the setup page.")]

# Step 3
story += step_title(3, "Other ways to set it up: phone or microSD card")
story += [P("If you used the Windows installer, it is already set up: skip to step 4 or 5. These options are for "
            "after installing with Arduino IDE, or to change the WiFi later."),
          P("3.1 · From your phone or computer", "h2"),
          side(I + "cyd-0_configuracion.png", 7 * cm, [
              *steps("Join the open WiFi network <b>YT-Dashboard</b>. On the touch screen you can simply <b>scan "
                     "the QR code</b> with your phone camera.",
                     "The setup page opens by itself. If not, go to <b>192.168.4.1</b>.",
                     "Choose the language (<b>Idioma · Language</b> → English), your WiFi, its password, your "
                     "channel (@handle) and paste your key from step 1.",
                     "Click <b>Guardar</b> (Save). The counter connects and shows your stats within seconds."),
          ]),
          warn("<b>Always</b> type the WiFi password: the page does not remember it.",
               "If your phone says \"this network has no Internet\", choose to stay connected.",
               "On a brand new counter the setup page is in Spanish; once you pick English the counter, its "
               "settings page and its alerts switch to English."),
          P(f"3.2 · With the microSD card {TOUCH}", "h2"),
          P("Handy to give it away ready to use or to set it up without a phone:"),
          *steps("On the computer, create a text file called <b>config.txt</b> in the root of the card (the "
                 "project has an example in <b>tarjeta-sd/config.txt.ejemplo</b>) with:"),
          codeblock("wifi=YourWiFiName\npassword=YourWiFiPassword\nchannel=@yourchannel\napi_key=AIza...\n"
                    "language=en"),
          *steps("Put the card in the counter and turn it on. It shows \"Setup from the SD card\" with what it "
                 "applied.",
                 "The file is renamed to <b>config_aplicada.txt</b> and the password is masked, so it is not "
                 "applied on every start and does not stay written down.", start=2),
          tip("The Spanish keys (wifi, contrasena, canal, clave_api, idioma) work too."),
          P("3.3 · Changing the settings later", "h2"),
          table([
              ["What to change", "How"],
              ["The channel, the key, the language, brightness, alerts…", "On its settings page (step 8), without "
               "touching the device."],
              ["The WiFi network", "Hold <b>BOOT</b> for 3 seconds and repeat 3.1; or in the installer, <b>Only "
                                   "change the settings</b>; or with config.txt."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm])]

# Step 4 · OLED
story += step_title(4, "Everyday use of the OLED counter")
story += [grid([I + "oled-1_suscriptores.png", I + "oled-2_vistas.png", I + "oled-3_video.png",
                I + "oled-4_grafico.png"], 7.6 * cm),
          table([
              ["Action", "What it does"],
              ["Short press on <b>BOOT</b>", "Goes to the next view. After the last one, <b>SETTINGS</b> shows the "
                                            "address of the settings page."],
              ["Hold <b>BOOT</b> for 3 seconds", "Opens the setup (to change the WiFi)."],
              ["Unplug and plug in again", "It restarts and reconnects by itself, without losing anything."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          *bullets("Subscribers are refreshed every 2 minutes and the latest video every 10.",
                   "The <b>FASTEST GROWING</b> view shows which of your 10 latest videos gains the most views in "
                   "24 h, how many, and how many times the average of the others. It shows up an hour after "
                   "turning it on (until then it compares fewer hours).",
                   "A <b>!</b> in the yellow strip means the last check failed (no Internet or quota used up). It "
                   "recovers by itself.",
                   "From 0:00 to 7:00 the brightness goes to the minimum (can be turned off in the settings).",
                   "Every minute the content shifts by 1 pixel so the OLED screen does not \"burn in\".")]

# Step 5 · touch
story += [PageBreak()]
story += step_title(5, "Everyday use of the touch screen")
story += [P("At the top you see your channel name and the time (in <b>amber</b>, with a dot, if the data could not "
            "be refreshed). At the bottom, some dots show which view you are on. It switches view by itself "
            "every 6 seconds."),
          table([
              ["Action", "What it does"],
              ["Tap an empty area, or BOOT", "Goes to the next view. After the last one, <b>Settings</b> shows a "
                                            "QR that opens the settings page."],
              ["Tap something with a red border and \"›\"", "Opens more details (table below)."],
              ["Hold <b>BOOT</b> for 3 seconds", "Opens the setup (to change the WiFi)."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          P("5.1 · What you can tap", "h2"),
          P("Everything you can tap looks the same: a box with a <b>red border and \"›\"</b>. Until you have used "
            "it once, the view shows a hint underneath (\"Tap the box: last 30 days\") and the border pulses when "
            "it appears; once you use it, the hint is gone for good."),
          table([
              ["Where you tap", "What appears"],
              ["The latest video's <b>comments</b> box", "The most recent comments, 2 at a time (tap for more)."],
              ["The <b>title</b> of the latest video or of \"fastest growing\"", "Your 10 latest videos, 4 at a "
               "time: views, the ones gained in 24 h (in green) and engagement. Tap top right to sort them by "
               "recent or by fastest growing."],
              ["A chart <b>bar</b>, or \"+… today ›\"", "That day's views compared with the average. Tap other bars "
                                                         "to change day."],
              ["Subscribers' \"+… in 7 days ›\"", "How subscribers changed over the last 30 days."],
          ], [6 * cm, CONTENT_W - 6 * cm]),
          Spacer(1, 6),
          grid([I + "cyd-d1_comentarios.png", I + "cyd-d2_videos.png", I + "cyd-d3_dia.png",
                I + "cyd-d4_suscriptores.png"], 7.4 * cm,
               captions=["Comments (new ones with a red dot)", "Your 10 latest videos", "Views of one day",
                         "Subscribers over 30 days"]),
          tip("To go back, tap \"‹\" at the top or any empty area. If you do not tap anything, it goes back by "
              "itself after 30 s.",
              "Comments are requested from YouTube when you tap them (1 quota unit). Videos are already there: "
              "they are checked every 30 minutes.")]
story += [P("5.2 · Fastest growing", "h2"),
          side(I + "cyd-3b_tendencia.png", 7 * cm, [
              P("Compares your <b>10 latest videos</b> and shows the one that gained the most views in the last 24 "
                "hours: how many (in green), how many times the average of the others (\"x3.2 your average\") and "
                "its <b>engagement</b> (likes and comments per 100 views)."),
              P("After turning it on, the comparison starts after an hour and covers more hours until it reaches "
                "24. Tap the title to see the 10 videos sorted by fastest growing.", "body"),
          ]),
          grid([I + "cyd-d2_videos.png", I + "cyd-d2b_videos_crecen.png"], 7.4 * cm,
               captions=["Your videos, most recent first", "… or fastest growing first"])]

# Step 6 · alerts
story += step_title(6, "Alerts, effects and ambient light", "")
story += [P(f"{TOUCH} &nbsp; After every check, the counter compares with the previous one and announces the most "
            "important thing:"),
          table([
              ["Event", "Screen", "LED", "Sound"],
              ["Milestone (100, 1,000, 10,000… subscribers; new first digit of views)", "Card with confetti",
               "Rainbow", "Fanfare"],
              ["Subscribers go up", "The number counts up and \"+N\" floats", "3 green flashes", "3 notes"],
              ["New comment on the latest video", "Blue alert with author and text (tap to read)",
               "Double blue pulse until you see them", "\"Ding\""],
              ["New video", "Red alert with the title", "3 red flashes", "3 notes"],
          ], [5 * cm, 5 * cm, 3.4 * cm, CONTENT_W - 13.4 * cm]),
          Spacer(1, 6),
          grid([I + "cyd-d8_hito.png", I + "cyd-d9_subida.png", I + "cyd-e1_aviso_comentario.png",
                I + "cyd-d6_comentarios_nuevos.png"], 7.4 * cm,
               captions=["Milestone with confetti (animated)", "Subscribers counting up", "New comment alert",
                         "Unread comments"]),
          *bullets("To see them without waiting: on the settings page, the <b>Milestone</b>, <b>Subscriber</b>, "
                   "<b>Comment</b> and <b>Video</b> buttons (they use your channel's data).",
                   "<b>Sound</b> needs a speaker on the \"Speaker\" connector. Volume in the settings (0 = muted).",
                   "With \"At night\" on, from 0:00 to 7:00 there is no sound and no LED."),
          P("6.1 · Ambient light", "h2"),
          P("When there are no alerts, the LED on the back sets the mood. Choose it in the settings and see it "
            "change live while you adjust it:"),
          table([
              ["Setting", "Options"],
              ["Effect", "Off · Steady · Breathing · Slow rainbow · Follows the channel (green if views are up "
                         "today, amber if not)"],
              ["Color", "YouTube red, warm white, blue, green, purple, orange, pink, cyan… or any"],
              ["Intensity and speed", "5-100 % · slow, normal or fast"],
              ["Blink when offline", "An amber blink every 10 s if it loses Internet (can be turned off)"],
          ], [4.5 * cm, CONTENT_W - 4.5 * cm]),
          Spacer(1, 6),
          tip("If the colors do not match the page, adjust the <b>white balance</b> (Ambient light → \"Adjust the "
              "LED colors\"): move the sliders until the LED looks pure white. The factory values (192/255/82) "
              "were measured with a camera on this board.")]

# Step 7 · SD and battery
story += step_title(7, "microSD card, sounds and battery")
story += [P(f"{TOUCH} &nbsp; The card is optional: without it everything works the same. It is read at start-up, "
            "so insert it with the counter unplugged."),
          P("7.1 · Prepare the card (FAT32)", "h2"),
          P("The counter only reads cards in <b>FAT32</b>. Cards of 32 GB or less already come that way. Cards of "
            "64 GB or more come in exFAT and Windows does not offer FAT32 for them; make a 32 GB partition like "
            "this:"),
          *steps("Put the card in the computer and copy anything you want to keep: <b>this erases it</b>.",
                 "Right-click the Start button → <b>Disk Management</b>.",
                 "Find the card (the \"Removable\" disk the size of the card; make sure it is not your hard drive). "
                 "Right-click its partition → <b>Delete Volume</b> → Yes.",
                 "Right-click \"Unallocated\" → <b>New Simple Volume</b> → size <b>32000</b> MB → file system "
                 "<b>FAT32</b> → Finish."),
          P("7.2 · What it does with the card", "h2"),
          table([
              ["File", "What for"],
              ["historial.csv", "Written by the counter: one line per check (date, subscribers, views and latest "
                                "video data). Opens in Excel. Download it from the settings page."],
              ["config.txt", "Set it up without a phone (step 3.2)."],
              ["sonidos/hito.wav, suscriptor.wav, comentario.wav, video.wav", "Your own sounds for each alert "
               "(milestone, subscriber, comment, video) instead of the built-in ones."],
              ["firmware.bin", "Update the program (step 9)."],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          tip("For the sounds: mono WAV, 16-22 kHz, 15 s at most. With Audacity: open your sound, <b>Tracks → Mix "
              "→ Mix Stereo Down to Mono</b>, project rate 22050 Hz bottom left and <b>File → Export → Export as "
              "WAV</b> (16-bit). MP3 is not supported."),
          P("7.3 · Battery", "h2"),
          P("The <b>BAT</b> connector takes a 3.7 V LiPo battery (1.25 mm JST plug); the board charges it when USB "
            "is connected. The board cannot tell by itself whether a battery is there, so you have to say so: on "
            "the settings page, tick <b>I have a battery connected</b>. The header then shows the level and, at "
            "10 %, reminds you to charge it."),
          grid([I + "cyd-e2_bateria.png", I + "cyd-e3_bateria_baja.png"], 7.4 * cm,
               captions=["Battery level in the header", "Low battery (in red)"]),
          warn("With the screen always on, a small battery lasts <b>hours</b>, not days: it is for moving it "
               "around for a while, not for leaving it unplugged.",
               "Mind the connector polarity (red = +). A reversed battery can damage the board.")]

# Step 8 · web settings
story += [PageBreak()]
story += step_title(8, "Settings from the browser")
story += [P("The counter has its own settings page on your network. No need to go through the initial setup "
            "again or type the WiFi password."),
          *steps("With your phone or computer on <b>the same WiFi</b> as the counter, open "
                 "<b>contador-xxxx.local</b> in the browser (the installer gave it to you). The counter's "
                 "<b>Settings</b> view shows it too, with its numeric address; on the touch screen, with a QR.",
                 "Change whatever you like. Brightness, volume and the light change live while you move them.",
                 "Click <b>Save</b>. It applies without restarting and is kept even if you unplug it."),
          grid([I + "ajustes-web-1.png", I + "ajustes-web-2.png"], 6.6 * cm),
          P("The touch screen's settings page, top to bottom (the OLED one has no alerts, light or card "
            "sections).", "caption"),
          table([
              ["Setting", "Values"],
              ["Language · Idioma", "English or Español: the screen and the page switch when you save"],
              ["Brightness · Switch view every", "5-100 % · 3-60 s or \"Never\""],
              ["At night", "From 0:00 to 7:00, lowest brightness and (touch) no alerts"],
              ["Alert volume · LED flashes", "0-100 % (0 = muted) · yes/no"],
              ["Battery", "Tick it if a battery is connected"],
              ["Try an alert", "Milestone, Subscriber, Comment, Video"],
              ["Ambient light", "Step 6.1, with the white balance"],
              ["microSD card", "Free space and history download"],
              ["Channel · Check every · Key", "@handle or link · 1-1440 min (2 by default) · empty = keep the "
               "current one"],
              ["Update firmware", "Link at the bottom (step 9)"],
          ], [5.5 * cm, CONTENT_W - 5.5 * cm]),
          Spacer(1, 6),
          tip("If <b>contador-xxxx.local</b> does not open (it happens on some Android phones), use the numeric "
              "address from the counter's Settings view.",
              "If the browser warns \"Not secure\" or \"Secure Site Not Available\", click <b>Continue to the "
              "HTTP site</b>: the page is on your own network.",
              "The page never shows the full key or the WiFi password.")]

# Step 9 · update
story += step_title(9, "Update the firmware", "2 min")
story += [P("When a new version comes out, it installs without a cable and without losing the settings. The "
            "installed version is shown at the bottom of the settings page (for example \"Firmware 1.6.0 "
            "(cyd)\")."),
          P("From the settings page (both models)", "h2"),
          *steps("Download the file for your model: <b>contador-cyd-v….bin</b> (touch screen) or "
                 "<b>contador-oled-v….bin</b> (OLED). Do not use the one ending in \"-completo\".",
                 "On the settings page, at the bottom, click <b>Update firmware</b>.",
                 "Choose the file and click <b>Install</b>. The counter shows a progress bar, restarts in about 15 "
                 "seconds and the page goes back to the settings."),
          P(f"With the microSD card {TOUCH}", "h2"),
          *steps("Copy the file to the root of the card and rename it to <b>firmware.bin</b>.",
                 "Insert the card and turn the counter on: it installs with a progress bar and restarts. The file "
                 "is renamed to firmware_instalado.bin."),
          tip("It is safe: the new firmware is written to a separate slot and is only activated if it arrived "
              "complete and is for the same model. If you pick the other board's file or the power goes off, the "
              "previous one keeps working.",
              "The Windows installer works too, but <b>Reinstall</b> erases the settings and the history stored "
              "on the device (not the one on the card)."),
          ]

# Step 10 · case
story += step_title(10, "Put it on your desk (optional)")
story += [*bullets("Touch screen: there are many 3D-printable cases for the 2.8\" \"ESP32-2432S028\" or \"Cheap "
                   "Yellow Display\". Leave access to the USB-C, the BOOT button and the card slot.",
                   "OLED: \"ESP32 + 0.96\" OLED\" cases, or a small cardboard or wooden box with the screen stuck "
                   "on with double-sided tape.",
                   "Hide the cable behind the monitor."),
          photo("The finished counter on the desk (cover photo).",
                "A close-up tapping the screen and the comment alert.")]

# Test checklist
story += [PageBreak(), P("Test checklist", "h1"),
          P("Tick each test as you check it. Those marked (T) are for the touch screen only.")]
tests = [
    ("Installer", "Follow step 2", "Ends with \"Your counter is up and running\""),
    ("Correct data", "Compare with your channel on YouTube", "Subscribers and views match (rounded)"),
    ("Rotation", "Watch for 30 seconds", "Switches view every 6 seconds"),
    ("Short press", "Press BOOT once", "Goes to the next view"),
    ("Long press", "Hold BOOT for 3 seconds", "The setup and the YT-Dashboard network appear"),
    ("Restart", "Unplug and plug in again", "Connects by itself, asks for nothing"),
    ("No Internet", "Turn off the router for 10 minutes", "Shows it is not up to date and recovers by itself"),
    ("Settings", "Open contador-xxxx.local and move the brightness", "The screen changes right away"),
    ("Language", "Pick Español / English in the settings and save", "The screen and the page switch language"),
    ("Fastest growing", "Look at that view an hour after turning it on", "Shows the video gaining the most views"),
    ("(T) Taps", "Tap comments, title, a bar and \"+… in 7 days\"", "The four detail views open"),
    ("(T) Hints", "Use an area", "Its hint disappears and does not come back"),
    ("(T) Alerts", "Test buttons in the settings", "Confetti, counting number and sliding alerts"),
    ("(T) Light", "Change effect, color and intensity", "The LED changes right away; white with no tint"),
    ("(T) Card", "Insert a FAT32 microSD and wait 5 min", "It shows in the settings and the history downloads"),
    ("Update", "Upload the .bin in Update firmware", "Restarts with the new version"),
    ("After 1-2 days", "Look at views, chart and subscribers", "\"+… today\", the chart and the change show up"),
    ("Night mode", "Look at the screen after 0:00", "The brightness is lower"),
]
rows = [["", "Test", "How", "Expected result", "Notes"]]
rows += [[CheckBox(), f"<b>{n}</b>", how, exp, ""] for n, how, exp in tests]
story += [table(rows, [0.8 * cm, 3.1 * cm, 4.6 * cm, 5.2 * cm, CONTENT_W - 13.7 * cm]), PageBreak()]

# Troubleshooting
story += [P("Troubleshooting", "h1"),
          table([
              ["Symptom", "Likely cause", "Fix"],
              ["\"Windows protected your PC\" when opening the installer", "The installer is not signed",
               "More info → Run anyway (only if you downloaded it from the official page)"],
              ["The installer or the computer does not detect the board", "Charge-only cable or missing CH340 driver",
               "Another USB-C cable; install the CH340 with the installer's button"],
              ["\"Check\" says the key is not valid", "Badly copied, API not enabled or just created",
               "Go over step 1; wait 5 minutes after creating it"],
              ["It does not connect to my WiFi", "5 GHz network or wrong password",
               "Use the router's 2.4 GHz network (usually the same name without \"5G\" or \"PLUS\")"],
              ["Slow to connect (\"Attempt 2 of 12\")", "Some routers refuse reconnection for a while",
               "It is normal: it usually connects on the second or third attempt"],
              ["\"Channel not found\"", "Misspelled name", "Use the exact @handle from the channel's address"],
              ["\"Invalid API key\" on the screen", "Key deleted, not enabled or badly pasted",
               "Enter a new one on the settings page"],
              ["The time shows in amber (or \"!\" on the OLED)", "No Internet or quota used up",
               "It recovers by itself; the quota resets at midnight Pacific time"],
              ["contador-xxxx.local does not open", "The phone does not resolve .local names or is on another network",
               "Use the numeric address from the Settings view; same WiFi"],
              ["The screen is in Spanish", "Spanish is the default language",
               "Settings page → Language · Idioma → English → Save (or language=en in config.txt)"],
              ["(T) Taps do not land where I touch", "Different touch calibration on your board",
               "See \"Calibrating the touch\" in the project's README"],
              ["(T) The card does not show up", "Not FAT32, or inserted while powered on",
               "Step 7.1; insert it unplugged"],
              ["(T) The LED colors are off", "Every LED is a bit different", "Adjust the white balance (step 6.1)"],
              ["(T) No sound", "No speaker or volume at 0", "Connect an 8 Ω speaker; raise the volume"],
              ["\"Not installed: this file is not…\"", "Firmware for the other board",
               "Download the .bin for your model (cyd = touch, oled = OLED)"],
              ["Arduino IDE: \"Sketch too big\"", "Default partition scheme",
               "Partition Scheme → Minimal SPIFFS (step 2B.4)"],
              ["Arduino IDE: \"LovyanGFX.hpp / U8g2lib.h: No such file\", or a black touch screen",
               "Your model's library is missing, or BOARD_CYD is not enabled", "Steps 2B.1 and 2B.3"],
          ], [4.8 * cm, 5 * cm, CONTENT_W - 9.8 * cm]),
          PageBreak()]

# How it works
story += [P("How it works", "h1"),
          *steps("When it starts it reads its settings (WiFi, channel, key, language, preferences) from internal "
                 "memory. For a second it listens on the USB cable in case the installer sends new ones.",
                 "If it has no settings, it creates the YT-Dashboard network with a captive portal (WiFiManager).",
                 "It joins the WiFi and syncs the time over the Internet (NTP).",
                 "It queries the YouTube Data API v3 over HTTPS, verifying Google's certificate.",
                 "It keeps a daily record to work out views per day and growth, and compares each check with the "
                 "previous one for the alerts.",
                 "It draws the views (U8g2 on the OLED, LovyanGFX on the touch screen) and repeats every few "
                 "minutes."),
          P("Quota usage", "h2"),
          table([
              ["Request", "How often", "Units per day"],
              ["Channel data", "Every 2 minutes (default)", "720"],
              ["Latest video (2 requests)", "Every 10 minutes", "288"],
              ["Your 10 latest videos (2 requests)", "Every 30 minutes", "96"],
              ["Comments", "Only when tapped (touch)", "1 each time"],
              ["<b>Total per counter</b>", "", "<b>≈ 1,100</b> (free limit: 10,000 per key)"],
          ], [6 * cm, 4.5 * cm, CONTENT_W - 10.5 * cm]),
          Spacer(1, 6),
          P("Privacy", "h2"),
          P("Only the channel's public data is queried; it never signs in to any account. The key and the WiFi "
            "password are stored only on the device; the installer sends them over the USB cable."),
          P("Project structure", "h2"),
          codeblock("ContadorYouTube/           the sketch you open in Arduino IDE\n"
                    "  ContadorYouTube.ino      main program (OLED and touch)\n"
                    "  config.h                 which model is compiled\n"
                    "  src/settings, portal     settings and captive portal\n"
                    "  src/lang.h               English / Spanish texts (TR macro)\n"
                    "  src/provision            setup over USB and config.txt\n"
                    "  src/youtube, certs.h     HTTPS requests to YouTube\n"
                    "  src/history              daily record\n"
                    "  src/oled/, src/cyd/      drawing for each screen\n"
                    "  src/alerts               LED and sound (touch)\n"
                    "  src/sdcard, battery      card and battery (touch)\n"
                    "  src/webui, updater       settings page and update\n"
                    "tools/instalador/          Windows installer\n"
                    "tools/                     previews, the manuals and build_firmware.py"),
          PageBreak()]

# Instructable
story += [P("Notes for the Instructable", "h1"),
          table([
              ["Item", "Suggestion"],
              ["Title", "Touch screen YouTube subscriber counter for $20 (ESP32 \"Cheap Yellow Display\")"],
              ["Intro", "What it does, what it costs, no soldering, and a GIF with the alerts and the confetti."],
              ["Steps", "Parts → API key → Installer (or Arduino IDE) → Use and taps → Alerts and light → Card "
                        "and battery → Case"],
              ["Files", "Link to the repository and the installer; the case STL if you make one."],
              ["Tags", "ESP32, CYD, YouTube, Arduino, IoT, touch screen, desk gadget, subscriber counter"],
          ], [3.5 * cm, CONTENT_W - 3.5 * cm]),
          Spacer(1, 6),
          P("Before publishing", "h2"),
          *bullets("Check that <b>secrets.h is not</b> in the repository or in the published files.",
                   "Record a short video: tapping the comments, the number counting up and the confetti.",
                   "Build a second counter following only this manual, to make sure nothing is missing.",
                   "Make it clear that every reader creates their own API key, for free.")]

# Appendix A
story += [PageBreak(), P("APPENDIX A", "kicker"), P("Publishing the firmware and the installer", "h1"),
          P("For whoever distributes the counter. The published firmware <b>never carries an API key</b>: every "
            "user enters their own. The key in <b>secrets.h</b> is only for your own tests."),
          *steps(f"Bump the version number in {code('src/version.h')} ({code('FIRMWARE_VERSION')}).",
                 f"Run {code('python tools/build_firmware.py')}. It compiles both boards without the key "
                 "(PUBLIC_BUILD), checks that it did not slip in and leaves in <b>dist/</b>, per board, the update "
                 ".bin and the -completo.bin (install from scratch).",
                 f"Run {code('python tools/instalador/build_exe.py')}: it creates "
                 "<b>dist/InstaladorContadorYouTube.exe</b> with the new firmware inside (about 18 MB).",
                 "Publish the installer and the two update .bin files, with their SHA-256 fingerprints."),
          tip("To stop Windows from showing \"Windows protected your PC\", the installer has to be signed with a "
              "code signing certificate (paid), or distributed through the Microsoft Store.",
              "Each board has its model written in the firmware: the update rejects the other one's.")]
story += [P("Licenses", "h2"),
          table([["Part", "License", "File"],
                 ["Code and firmware", "PolyForm Noncommercial 1.0.0: use, changes and copies for non-commercial "
                  "purposes; to sell, the author's permission", "LICENSE.md"],
                 ["Windows installer", "GPL-2.0-or-later (it bundles esptool, which is GPL)",
                  "tools/instalador/LICENSE"],
                 ["This manual", "CC BY-NC 4.0: copy and adapt crediting the author, non-commercial",
                  "LICENCIA-MANUAL.txt"],
                 ["Libraries and fonts", "Each author's (MIT, BSD, LGPL, OFL…)", "AVISOS-TERCEROS.txt"]],
                [3.4 * cm, 8.6 * cm, 5 * cm]),
          P("Whoever shares the code or the firmware must include those files (or link to them) and the notice "
            "\"Required Notice: Copyright (c) 2026 Adrian Cubas\".", "small")]

# Appendix B
story += [CondPageBreak(12 * cm), P("APPENDIX B", "kicker"), P("E-ink version", "h1"),
          P("The same project includes a third, experimental version for the <b>LilyGo T5 4.7\" ESP32-S3</b> "
            "(960×540, 16 grays, about €40). It runs on a battery for weeks because it sleeps between updates "
            "and the ink keeps the image without using power."),
          image("preview-dashboard.png", 13 * cm, "Approximate preview of the e-ink panel (Spanish only for now)."),
          *bullets("Buy the <b>regular ESP32-S3</b> version, not the \"Pro\" (it is different hardware).",
                   f"It is uploaded with PlatformIO (VS Code): {code('pio run -e t5-epd47-s3 -t upload')}.",
                   "It has no installer, no alerts, no over-the-air update and no English yet.")]

# -------------------------------------------------------------------- PDF ---

doc = SimpleDocTemplate(str(OUT), pagesize=A4, leftMargin=MARGIN, rightMargin=MARGIN, topMargin=MARGIN,
                        bottomMargin=2.2 * cm, title="Desktop YouTube counter",
                        author="Adrian Cubas", subject="Build and user manual")
doc.build(story, onFirstPage=on_page, onLaterPages=on_page)
print(OUT)
