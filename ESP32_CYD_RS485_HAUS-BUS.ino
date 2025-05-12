/*  ESP32 ST7789 Touch-Menü
    Für TZT ESP32 2.4" LCD mit ST7789 Controller und XPT2046 Touchscreen
    Enthält ein 6-Tasten-Menü, PWM-gesteuerte Hintergrundbeleuchtung und
    regelmäßige Statusmeldungen.
*/

#include "config.h"
#include "touch.h"
#include "display.h"
#include "menu.h"
#include "communication.h"
#include "backlight.h"
#include "led.h"  // Neue Datei für LED-Steuerung einbinden

// Timing für Hintergrundbeleuchtungs-Status
unsigned long lastBacklightStatusTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 ST7789 Touch-Menü");
  
  // Initialisiere die RGB-LED
  setupLed();
  
  // Initialisiere Kommunikation (UART und USB)
  setupCommunication();
  
  // Initialisiere Hintergrundbeleuchtung
  setupBacklight();
  setBacklight(DEFAULT_BACKLIGHT);
  
  // Initialisiere den Touchscreen
  setupTouch();
  
  // Initialisiere das Display
  setupDisplay();

  // Anzeige einiger Infos
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("ESP32 ST7789 mit XPT2046", SCREEN_WIDTH/2, 20, 2);
  tft.drawCentreString("Initialisierung...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 2);
  tft.drawCentreString("Version 2.0", SCREEN_WIDTH/2, SCREEN_HEIGHT - 30, 1);
  
  // Erste Statusmeldung der Hintergrundbeleuchtung
  sendBacklightStatus();
  lastBacklightStatusTime = millis();
  
  delay(1000);
  
  // Gehe direkt zum Menü
  showMenu();
}

void loop() {
  // LED-Status aktualisieren
  updateLedStatus();
  
  // Prüfen, ob ein Statusupdate für die Hintergrundbeleuchtung fällig ist
  unsigned long currentMillis = millis();
  if (currentMillis - lastBacklightStatusTime >= BACKLIGHT_STATUS_INTERVAL) {
    sendBacklightStatus();
    lastBacklightStatusTime = currentMillis;
  }
  
  // Überprüfe, ob Telegramme über UART empfangen wurden
  processIncomingTelegrams();
  
  // Überprüfe Touchscreen-Berührungen
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    delay(50); // Entprellung
    
    // Im Menü-Modus
    int x, y;
    getTouchPoint(&x, &y);
    
    // Prüfen, ob der Test-Button gedrückt wurde
    if (x >= SCREEN_WIDTH - 60 && x <= SCREEN_WIDTH - 5 && y >= 5 && y <= 35) {
      // Test-Button wurde gedrückt
      Serial.println("Test-Button gedrückt");
      testTouch();
    } else {
      int buttonPressed = checkButtonPress(x, y);
      
      if (buttonPressed >= 0) {
        // Button wurde gedrückt
        Serial.print("Button gedrückt: ");
        Serial.println(buttons[buttonPressed].label);
        
        // Taster-Status auf gedrückt setzen
        buttons[buttonPressed].pressed = true;
        
        // Sende Taster-gedrückt-Telegramm
        sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "1");
        
        // Speichere den ursprünglichen Aktivierungszustand
        bool wasActive = buttons[buttonPressed].isActive;
        
        // Button visuell als gedrückt anzeigen (grün hinterlegen)
        setButtonActive(buttonPressed, true);
        
        // Warte, bis der Touchscreen losgelassen wird
        while (touchscreen.touched()) {
          delay(10);
        }
        
        // Button wieder normal zeichnen, ABER NUR wenn er vorher nicht aktiv war
        if (!wasActive) {
          setButtonActive(buttonPressed, false);
        }
        
        // Tester-Button erneut zeichnen (blau)
        tft.fillRect(SCREEN_WIDTH - 60, 5, 55, 30, TFT_BLUE);
        tft.setTextColor(TFT_WHITE);
        tft.drawCentreString("TEST", SCREEN_WIDTH - 32, 15, 1);
        
        // Taster-Status auf losgelassen setzen
        buttons[buttonPressed].pressed = false;
        
        // Sende Taster-losgelassen-Telegramm
        sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "0");
      }
    }
  }
}