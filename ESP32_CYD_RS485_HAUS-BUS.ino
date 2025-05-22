/*  ESP32 ST7789 Touch-Menü mit CSMA/CD
    Version 2.4 - Mai 2025
    
    Neue Features:
    - CSMA/CD (Carrier Sense Multiple Access/Collision Detection)
    - Sendepuffer mit Prioritäten
    - Automatische Wiederholung bei Kollisionen
    - Backoff-Algorithmus
    - Korrigierte LED-Button-Zuordnung (LED 49-54 → Button 1-6)
*/

#include "config.h"
#include "touch.h"
#include "display.h"
#include "menu.h"
#include "communication.h"
#include "backlight.h"
#include "led.h"

// Timing für Hintergrundbeleuchtungs-Status
unsigned long lastBacklightStatusTime = 0;

// Neue Funktionen aus communication.cpp
extern void updateCommunication();
extern void printCommunicationStats();

void setup() {
  Serial.begin(115200);
  delay(100);
  
  #if DB_INFO == 1
    Serial.println("\nESP32 ST7789 Touch-Menü mit CSMA/CD - Start");
    Serial.println("Firmware-Version: 2.4");
    Serial.println("Datum: Mai 2025");
    Serial.println("Hardware: Separate UART2 RS485 mit CSMA/CD");
    Serial.println("LED-Button-Zuordnung: LED 49-54 → Button 1-6");
  #endif
  
  // Kommunikation initialisieren (mit CSMA/CD)
  setupCommunication();
  
  // Initialisiere die RGB-LED
  setupLed();
  
  // Initialisiere Hintergrundbeleuchtung
  setupBacklight();
  setBacklight(DEFAULT_BACKLIGHT);
  
  // Initialisiere den Touchscreen
  setupTouch();

  // Initialisiere die Button-IDs (wird jetzt in menu.cpp korrekt gemacht)
  initializeButtons();
  
  // Initialisiere das Display
  setupDisplay();

  // Anzeige einiger Infos
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("ESP32 ST7789 mit CSMA/CD", SCREEN_WIDTH/2, 20, 2);
  tft.drawCentreString("Initialisierung...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 2);
  tft.drawCentreString("Version 2.4", SCREEN_WIDTH/2, SCREEN_HEIGHT - 30, 1);
  
  // Erste Statusmeldung der Hintergrundbeleuchtung
  sendBacklightStatus();
  lastBacklightStatusTime = millis();
  
  delay(1000);
  
  // Gehe direkt zum Menü
  showMenu();
}

void initializeButtons() {
  // KORRIGIERT: Die IDs werden jetzt korrekt in menu.cpp/initButtons() gesetzt
  // Button-Index 0-5 entspricht instanceID "17"-"22"
  // LED-IDs 49-54 werden in communication.cpp auf Button-Index 0-5 gemappt
  
  #if DB_INFO == 1
    Serial.println("\n=== Button-LED-Zuordnung ===");
    for (int i = 0; i < NUM_BUTTONS; i++) {
      Serial.print("Button ");
      Serial.print(i + 1);
      Serial.print(" (Index ");
      Serial.print(i);
      Serial.print(") → BTN.");
      Serial.print(buttons[i].instanceID);
      Serial.print(" ↔ LED.");
      Serial.println(49 + i);
    }
    Serial.println("============================");
  #endif
}

void loop() {
  // LED-Status aktualisieren
  updateLedStatus();
  
  // *** NEU: Kommunikation mit CSMA/CD verwalten ***
  updateCommunication();
  
  // Prüfen, ob ein Statusupdate für die Hintergrundbeleuchtung fällig ist
  unsigned long currentMillis = millis();
  if (currentMillis - lastBacklightStatusTime >= BACKLIGHT_STATUS_INTERVAL) {
    sendBacklightStatus();
    lastBacklightStatusTime = currentMillis;
  }
  
  // Touch-Eingaben verarbeiten
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    delay(50); // Entprellung
    
    int x, y;
    getTouchPoint(&x, &y);
    
    // Prüfen, ob der Test-Button gedrückt wurde
    if (x >= SCREEN_WIDTH - 60 && x <= SCREEN_WIDTH - 5 && y >= 5 && y <= 35) {
      #if DB_INFO == 1
        Serial.println("DEBUG: Test-Button gedrückt");
      #endif
      testTouch();
    } else {
      int buttonPressed = checkButtonPress(x, y);
      
      if (buttonPressed >= 0) {
        #if DB_INFO == 1
          Serial.print("DEBUG: Button gedrückt: ");
          Serial.print(buttons[buttonPressed].label);
          Serial.print(" (ID: ");
          Serial.print(buttons[buttonPressed].instanceID);
          Serial.println(")");
        #endif
        
        // Taster-Status auf gedrückt setzen
        buttons[buttonPressed].pressed = true;
        
        // *** KORRIGIERT: Sende mit korrekter instanceID (17-22) ***
        // sendTelegram nutzt die instanceID aus buttons[buttonPressed].instanceID
        sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "1");
        
        // Speichere den ursprünglichen Aktivierungszustand
        bool wasActive = buttons[buttonPressed].isActive;
        
        // Button visuell als gedrückt anzeigen (grün hinterlegen)
        setButtonActive(buttonPressed, true);
        
        // Warte, bis der Touchscreen losgelassen wird
        while (touchscreen.touched()) {
          delay(10);
          
          // *** WICHTIG: Während des Wartens die Kommunikation weiter verwalten ***
          updateCommunication();
        }
        
        // Button wieder normal zeichnen, ABER NUR wenn er vorher nicht aktiv war
        if (!wasActive) {
          setButtonActive(buttonPressed, false);
        }
        
        // Test-Button erneut zeichnen (blau)
        tft.fillRect(SCREEN_WIDTH - 60, 5, 55, 30, TFT_BLUE);
        tft.setTextColor(TFT_WHITE);
        tft.drawCentreString("TEST", SCREEN_WIDTH - 32, 15, 1);
        
        // Taster-Status auf losgelassen setzen
        buttons[buttonPressed].pressed = false;
        
        // *** KORRIGIERT: Sende Loslassen-Nachricht mit korrekter instanceID ***
        sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "0");
      }
    }
  }
  
  // Statistiken alle 60 Sekunden ausgeben (optional)
  static unsigned long lastStatsOutput = 0;
  if (millis() - lastStatsOutput > 60000) {
    printCommunicationStats();
    lastStatsOutput = millis();
  }
}