#ifndef CONFIG_H
#define CONFIG_H

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
// Falls möglich, versuchen ESP32-spezifische Bibliotheken für PWM-Funktionen einzubinden
#include "Arduino.h"
// Füge diese Definitionen zu config.h hinzu:

// RGB-LED Pins
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17

// UART Pins für die Hardware-Serielle Kommunikation gemäß TZT ESP32 2.4" Dokumentation
#define UART_RX_PIN 22  // RX Pin gemäß Doku
#define UART_TX_PIN 21  // TX Pin gemäß Doku
#define UART_EN_PIN 35  // Enable Pin gemäß Doku (falls benötigt)

// LED-Blinkzeiten
#define LED_SEND_DURATION 100  // Dauer des roten Signals beim Senden (ms)
#define LED_RECEIVE_DURATION 200  // Dauer des blauen Signals beim Empfangen (ms)

// Zustände für die LED
extern unsigned long ledEndTime;
extern int currentLedState;  // 0=aus, 1=rot (senden), 2=blau (empfangen)

// Externes TFT-Display
extern TFT_eSPI tft;

// Externes Touchscreen
extern XPT2046_Touchscreen touchscreen;

// Touchscreen Pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 13  // T_DIN
#define XPT2046_MISO 12  // T_OUT
#define XPT2046_CLK 14   // T_CLK
#define XPT2046_CS 33    // T_CS

// Hintergrundbeleuchtungs-Pin und Kanal
#define TFT_BL_PIN 27           // GPIO-Pin für Hintergrundbeleuchtung
#define TFT_BL_CHANNEL 0        // PWM-Kanal (0-15)
#define PWM_FREQ 5000           // PWM-Frequenz in Hz
#define PWM_RESOLUTION 8        // Auflösung in Bit (8 = 256 Stufen)
#define DEFAULT_BACKLIGHT 100   // Standard-Helligkeit (0-100%)

// Display-Dimensionen und Ausrichtung
#define PORTRAIT 0
#define LANDSCAPE 1
#define SCREEN_ORIENTATION LANDSCAPE

// Display-Dimensionen basierend auf Ausrichtung
#if SCREEN_ORIENTATION == PORTRAIT
  #define SCREEN_WIDTH 240
  #define SCREEN_HEIGHT 320
#else // LANDSCAPE
  #define SCREEN_WIDTH 320
  #define SCREEN_HEIGHT 240
#endif

// Konstanten für die Touch-Parameter, basierend auf typischen Werten für XPT2046
#define TOUCH_MIN_X 400
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 400
#define TOUCH_MAX_Y 3900

// Aktuelle Invertierungseinstellungen (extern in touch.cpp definiert)
extern bool invertTouchX;
extern bool invertTouchY;

// Kommunikationsprotokoll Konstanten
#define START_BYTE 0xFD
#define END_BYTE 0xFE
#define DEVICE_ID "5999"  // Geräte-ID

// Timing für Hintergrundbeleuchtungs-Status
#define BACKLIGHT_STATUS_INTERVAL 23000  // 23 Sekunden

// Button-Konfiguration
#define NUM_BUTTONS 6
typedef struct {
  int x;
  int y;
  int w;
  int h;
  String label;
  String instanceID;
  uint16_t color;
  uint16_t textColor;
  bool pressed;
  bool isActive;  // Neues Feld für den Aktivierungsstatus (für grüne Beleuchtung)
} Button;

// Externes Array für die Buttons
extern Button buttons[NUM_BUTTONS];
extern int buttonWidth, buttonHeight;

// Externe aktuelle Hintergrundbeleuchtung
extern int currentBacklight;

#endif // CONFIG_H