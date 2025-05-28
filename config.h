// config.h - KORRIGIERTE VERSION
#ifndef CONFIG_H
#define CONFIG_H

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <HardwareSerial.h>
#include "Arduino.h"
#include <FS.h>
using namespace fs;  // <- Damit 'FS' statt 'fs::FS' funktioniert

/*
TOUCH-MODI:
0 = LEGACY_MODE      - Alter Code, keine Änderungen
1 = NORMAL_MODE      - Wisch-Schutz + Auto-Reset (empfohlen)
2 = AUTO_RESET_ONLY  - Nur Auto-Reset, kein Wisch-Schutz
3 = SWIPE_ONLY       - Nur Wisch-Erkennung, kein Auto-Reset  
4 = SWIPE_APP_MODE   - Alles deaktiviert für Wisch-Anwendungen
5 = SENSITIVE_MODE   - Sehr empfindliche Wisch-Erkennung
*/
#define TOUCH_MODE 1    // ← NUR DIESE EINE ZEILE ÄNDERN!

#if TOUCH_MODE == 0
    // LEGACY_MODE - Originaler Code
    #define ENABLE_SWIPE_DETECTION 0
    #define AUTO_RESET_BUTTONS 0
    #define SWIPE_TIMEOUT_MS 0
    #define SWIPE_DISTANCE_THRESHOLD 0
    
#elif TOUCH_MODE == 1
    // NORMAL_MODE - Empfohlen für normale Nutzung
    #define ENABLE_SWIPE_DETECTION 1
    #define AUTO_RESET_BUTTONS 1
    #define SWIPE_TIMEOUT_MS 500
    #define SWIPE_DISTANCE_THRESHOLD 30
    
#elif TOUCH_MODE == 2
    // AUTO_RESET_ONLY - Nur Auto-Reset
    #define ENABLE_SWIPE_DETECTION 0
    #define AUTO_RESET_BUTTONS 1
    #define SWIPE_TIMEOUT_MS 1000
    #define SWIPE_DISTANCE_THRESHOLD 0
    
#elif TOUCH_MODE == 3
    // SWIPE_ONLY - Nur Wisch-Erkennung
    #define ENABLE_SWIPE_DETECTION 1
    #define AUTO_RESET_BUTTONS 0
    #define SWIPE_TIMEOUT_MS 0
    #define SWIPE_DISTANCE_THRESHOLD 50
    
#elif TOUCH_MODE == 4
    // SWIPE_APP_MODE - Für Wisch-Anwendungen
    #define ENABLE_SWIPE_DETECTION 0
    #define AUTO_RESET_BUTTONS 0
    #define SWIPE_TIMEOUT_MS 0
    #define SWIPE_DISTANCE_THRESHOLD 0
    
#elif TOUCH_MODE == 5
    // SENSITIVE_MODE - Sehr empfindlich
    #define ENABLE_SWIPE_DETECTION 1
    #define AUTO_RESET_BUTTONS 1
    #define SWIPE_TIMEOUT_MS 300
    #define SWIPE_DISTANCE_THRESHOLD 15
    
#else
    #error "TOUCH_MODE ungültig! Wähle 0-5"
#endif


// Debug-Einstellungen
#define DB_TX_HEX 0      // Hex-Ausgabe für gesendete Telegramme
#define DB_TX_INFO 0     // Allgemeine Informationen zum Senden
#define DB_RX_HEX 0      // Hex-Ausgabe für empfangene Bytes
#define DB_RX_INFO 0     // Allgemeine Informationen zum Empfang
#define DB_INFO 0        // Allgemeine Debug-Informationen

// RAW-Debug-Modus
#define RAW_DEBUG 0      // 0=normale Verarbeitung, 1=RAW-Modus

// RGB-LED Pins
#define LED_RED_PIN 4
#define LED_GREEN_PIN 17
#define LED_BLUE_PIN 16

// UART-Konfiguration für separate Schnittstellen
#define UART_RX_PIN 22   // RX Pin für UART2 (RS485)
#define UART_TX_PIN 21   // TX Pin für UART2 (RS485)

// Separate UART2-Instanz für RS485
extern HardwareSerial RS485Serial;

// LED-Timing
#define LED_SEND_DURATION 200    // Rot beim Senden (ms)
#define LED_RECEIVE_DURATION 100 // Blau beim Empfangen (ms)

// UART-Timing-Parameter
#define UART_TIMEOUT_MS 10       // UART-Timeout in Millisekunden
#define BYTE_TIMEOUT_MS 50       // Timeout zwischen Bytes (ms)
#define TELEGRAM_TIMEOUT_MS 50   // Timeout für komplettes Telegramm (ms)
#define INTER_FRAME_DELAY 5      // Verzögerung zwischen Frames (ms)

// CSMA/CD-Parameter (STATISCH - keine Division-durch-Null möglich)
#define BUS_IDLE_TIME_MS 10          // Zeit ohne Aktivität = Bus frei (ms)
#define COLLISION_DETECT_TIME_MS 5   // Zeit nach Sendebeginn für Kollisionsprüfung (ms)
#define MAX_TRANSMISSION_ATTEMPTS 3  // Maximale Sendeversuche pro Telegramm
#define SEND_QUEUE_SIZE 10           // Größe des Sendepuffers
#define MAX_RETRIES_PER_TELEGRAM 5   // Maximale Wiederholungen pro Telegramm

// Prioritätsstufen für verschiedene Nachrichtentypen
#define PRIORITY_CRITICAL 0      // Kritische Nachrichten (Notfälle)
#define PRIORITY_HIGH 1          // Hohe Priorität (Taster)
#define PRIORITY_NORMAL 5        // Normale Priorität (Standard)
#define PRIORITY_LOW 7           // Niedrige Priorität (Status)
#define PRIORITY_BACKGROUND 9    // Hintergrund (Statistiken)

// Backoff-Algorithmus Parameter
#define MIN_BACKOFF_TIME 5       // Minimale Backoff-Zeit (ms)
#define MAX_BACKOFF_TIME 100     // Maximale Backoff-Zeit (ms)
#define BACKOFF_MULTIPLIER 10    // Multiplikator pro Retry-Versuch

// Buffer-Größen
#define MAX_TELEGRAM_LENGTH 255  // Maximale Telegramm-Länge

// LED-Status-Variablen
extern unsigned long ledEndTime;
extern int currentLedState;

// Display und Touch
extern TFT_eSPI tft;
extern XPT2046_Touchscreen touchscreen;

// Touchscreen Pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 13
#define XPT2046_MISO 12
#define XPT2046_CLK 14
#define XPT2046_CS 33

// Hintergrundbeleuchtung
#define TFT_BL_PIN 27           // LED Hintergrundbeleuchtung
#define TFT_BL_CHANNEL 0        // PWM-Kanal
#define PWM_FREQ 5000           // PWM-Frequenz
#define PWM_RESOLUTION 8        // PWM-Auflösung
#define DEFAULT_BACKLIGHT 100   // Standardhelligkeit

// Display-Konfiguration - KORRIGIERT für USB rechts
#define PORTRAIT 0
#define LANDSCAPE 1

#define ROTATION_0   0   // Portrait
#define ROTATION_90  1   // Landscape 90° (USB links)
#define ROTATION_180 2   // Portrait 180°
#define ROTATION_270 3   // Landscape 270° (USB rechts)

#define SCREEN_ORIENTATION ROTATION_180       // Direkt PORTRAIT (0) setzen

// *** KORRIGIERTE If-Bedingung ***
#if SCREEN_ORIENTATION == ROTATION_0 || SCREEN_ORIENTATION == ROTATION_180 || SCREEN_ORIENTATION == PORTRAIT
  // Hochformat
  #define SCREEN_WIDTH 240
  #define SCREEN_HEIGHT 320
#else 
  // Querformat
  #define SCREEN_WIDTH 320
  #define SCREEN_HEIGHT 240
#endif

// Touch-Parameter
#define TOUCH_MIN_X 400
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 400
#define TOUCH_MAX_Y 3900

extern bool invertTouchX;
extern bool invertTouchY;

// Kommunikationsprotokoll
#define START_BYTE 0xFD        // Startbyte für Telegramme
#define END_BYTE 0xFE          // Endbyte für Telegramme
#define DEVICE_ID "5999"       // Eindeutige Geräte-ID (kann über Service-Manager geändert werden)

// Timing für Status-Updates
#define BACKLIGHT_STATUS_INTERVAL 23000  // Intervall für Backlight-Status

// *** WICHTIG: NUM_BUTTONS FRÜH DEFINIEREN ***
#define NUM_BUTTONS 6           // Anzahl der Bildschirmtaster

// Button-Datenstruktur
typedef struct {
  int x;                        // X-Position
  int y;                        // Y-Position
  int w;                        // Breite
  int h;                        // Höhe
  String label;                 // Beschriftung
  String instanceID;            // Instanz-ID (z. B. "17")
  uint16_t color;               // Hintergrundfarbe
  uint16_t textColor;           // Textfarbe
  bool pressed;                 // Taster gedrückt
  bool isActive;                // Status aktiv/inaktiv
} Button;

extern Button buttons[NUM_BUTTONS];
extern int buttonWidth, buttonHeight;
extern int currentBacklight;

// Buffer für Telegramm-Verarbeitung
extern char telegramBuffer[MAX_TELEGRAM_LENGTH];
extern int bufferPos;
extern bool receivingTelegram;

// Timing-Variablen
extern unsigned long lastByteTime;
extern unsigned long telegramStartTime;

// CSMA/CD Variablen
extern bool busIdle;
extern unsigned long lastBusActivity;

// Statistik-Variablen
extern unsigned long totalSent;
extern unsigned long totalCollisions;
extern unsigned long totalRetries;
extern unsigned long totalDropped;  // Verworfene Telegramme

// *** CONVERTER WEB SERVICE INCLUDE - JETZT NACH NUM_BUTTONS ***
#include "converter_web_service.h"

#endif // CONFIG_H