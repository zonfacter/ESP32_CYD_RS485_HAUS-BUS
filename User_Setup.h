
// Konfiguration für TZT ESP32 2.4" Display mit ST7789 Controller
// Diese Datei muss im TFT_eSPI/User_Setup.h platziert werden

// User defined information reported by "Read_User_Setup" test & diagnostics example
#define USER_SETUP_INFO "User_Setup for TZT ESP32 2.4 ST7789"

// Display-Treiber definieren
#define ST7789_DRIVER

// Bildschirmgröße
#define TFT_WIDTH  240
#define TFT_HEIGHT 320  // ST7789 240 x 320

// Farbordnung - BGR für die meisten ST7789 Displays
#define TFT_RGB_ORDER TFT_BGR

// Display-Inversion ausschalten (Farbinversion)
#define TFT_INVERSION_OFF

// DMA für ESP32 nutzen (beschleunigt die Anzeige)
#define ESP32_DMA

// Hintergrundbeleuchtungs-Pin und Zustand
#define TFT_BL   27            // LED Hintergrundbeleuchtungs-Pin
#define TFT_BACKLIGHT_ON HIGH  // Pegel zum Einschalten der Hintergrundbeleuchtung (HIGH oder LOW)

// SPI-Pins für TZT ESP32 mit ST7789 Display
#define TFT_MISO 12
#define TFT_MOSI 13  // In einigen Display-Treiberplatinen steht möglicherweise "SDA" oder so ähnlich
#define TFT_SCLK 14
#define TFT_CS   15  // Chip-Auswahl-Steuerpin
#define TFT_DC   2   // Data/Command-Steuerpin
#define TFT_RST  -1  // Reset-Pin (kann mit dem Arduino-RESET-Pin verbunden werden)

// Touchscreen CS-Pin
#define TOUCH_CS 33  // Chip-Auswahl-Pin (T_CS) des Touchscreens

// HSPI-Port verwenden
#define USE_HSPI_PORT

// Schriftarten laden
#define LOAD_GLCD   // Font 1. Original Adafruit 8 Pixel Font benötigt ~1820 Bytes im FLASH
#define LOAD_FONT2  // Font 2. Kleine 16 Pixel hohe Schrift, benötigt ~3534 Bytes im FLASH, 96 Zeichen
#define LOAD_FONT4  // Font 4. Mittlere 26 Pixel hohe Schrift, benötigt ~5848 Bytes im FLASH, 96 Zeichen
#define LOAD_FONT6  // Font 6. Große 48 Pixel Schrift, benötigt ~2666 Bytes im FLASH, nur Zeichen 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7-Segment 48 Pixel Schrift, benötigt ~2438 Bytes im FLASH, nur Zeichen 1234567890:-.
#define LOAD_FONT8  // Font 8. Große 75 Pixel Schrift benötigt ~3256 Bytes im FLASH, nur Zeichen 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative zu Font 8 oben, etwas schmaler, so dass 3 Ziffern auf ein 160-Pixel-TFT passen
#define LOAD_GFXFF  // FreeFonts. Enthält Zugriff auf die 48 Adafruit_GFX-Gratis-Schriften FF1 bis FF48 und benutzerdefinierte Schriften

// Aktivieren Sie Smooth Font
#define SMOOTH_FONT

// SPI-Frequenzen
#define SPI_FREQUENCY  80000000        // Standard-SPI-Frequenz

// Optionale reduzierte SPI-Frequenz zum Lesen des TFT
#define SPI_READ_FREQUENCY  80000000

// Der XPT2046 erfordert eine niedrigere SPI-Taktrate von 2,5 MHz, daher definieren wir diese hier:
#define SPI_TOUCH_FREQUENCY  2500000  //2500000