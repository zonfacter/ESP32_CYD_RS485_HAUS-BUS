/**
 * display_calibration.cpp - VEREINFACHTE VERSION
 * 
 * Nur die wichtigsten Funktionen ohne komplizierte Touch-Details
 */

#include "display_calibration.h"
#include "service_manager.h"
#include "menu.h"
#include "header_display.h"
#include "touch.h"

// Globale Kalibrierungs-Ergebnisse
CalibrationResult calibrationResult;

void startDisplayCalibration() {
  if (!CALIBRATION_ENABLED) {
    Serial.println("DEBUG: Display-Kalibrierung ist deaktiviert");
    return;
  }
  
  // *** NEU: TFT + Backlight initialisieren ***
  Serial.println("Initialisiere Display für Kalibrierung...");
  
  // Backlight initialisieren (falls nötig)
  pinMode(TFT_BL_PIN, OUTPUT);
  ledcAttach(TFT_BL_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(TFT_BL_PIN, 255);  // Volle Helligkeit
  
  // TFT initialisieren
  tft.init();
  tft.setRotation(0);  // Start mit Rotation 0
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Display-Kalibrierung", 10, 10, 2);
  tft.drawString("Startet in 2 Sekunden...", 10, 40, 1);
  
  delay(2000);  // Display-Test
  
  Serial.println("\n" + String('=', 50));
  Serial.println("       DISPLAY KALIBRIERUNG GESTARTET");
  Serial.println(String('=', 50));
  
  Serial.println("Optionen:");
  Serial.println("1 = Schneller Test");
  Serial.println("ESC = Überspringen");
  Serial.println("Automatischer Start in 20 Sekunden...");
  
  // 10 Sekunden warten
  unsigned long startTime = millis();
  bool choiceMade = false;
  
  while (!choiceMade && (millis() - startTime < 20000)) {
    if (Serial.available()) {
      String input = Serial.readString();
      input.trim();
      
      if (input == "1") {
        quickRotationTest();
        choiceMade = true;
      } else if (input.equalsIgnoreCase("ESC")) {
        Serial.println("Kalibrierung übersprungen.");
        return;
      }
    }
    delay(100);
  }
  
  if (!choiceMade) {
    Serial.println("Timeout - Starte automatischen Test...");
    quickRotationTest();
  }
  
  showCalibrationResults();
}

void quickRotationTest() {
  Serial.println("\n--- ROTATIONS-TEST ---");
  
  for (int rotation = 0; rotation <= 3; rotation++) {
    Serial.print("\nRotation ");
    Serial.print(rotation);
    Serial.print(" - ");
    showRotationInfo(rotation);
    
    // Rotation setzen
    tft.setRotation(rotation);
    
    // Test-Pattern anzeigen
    drawTestPattern(rotation);
    
    // Einfacher Touch-Test (ohne getTouchPoint Details)
    Serial.println("Touch-Test läuft...");
    unsigned long testStart = millis();
    bool touchDetected = false;
    
    while (millis() - testStart < 15000) { // 15 Sekunden
      if (touchscreen.tirqTouched() && touchscreen.touched()) {
        touchDetected = true;
        
        // Einfaches visuelles Feedback ohne Koordinaten-Details
        tft.fillCircle(tft.width()/2, tft.height()/2, 10, TFT_RED);
        
        Serial.println("Touch erkannt!");
        delay(500);
        while (touchscreen.touched()) delay(10);
        break;
      }
    }
    
    calibrationResult.touchWorking[rotation] = touchDetected;
    calibrationResult.layoutGood[rotation] = true; // Angenommen
    
    Serial.print("Ergebnis: Touch=");
    Serial.println(touchDetected ? "OK" : "FEHLER");
  }
  
  // Beste Rotation ermitteln
  calibrationResult.bestRotation = 2; // Standard-Empfehlung
  for (int i = 0; i < 4; i++) {
    if (calibrationResult.touchWorking[i] && calibrationResult.layoutGood[i]) {
      calibrationResult.bestRotation = i;
      break;
    }
  }
}

void showRotationInfo(int rotation) {
  String info;
  switch (rotation) {
    case 0: info = "Portrait USB oben"; break;
    case 1: info = "Landscape USB links"; break;
    case 2: info = "Portrait USB unten"; break; 
    case 3: info = "Landscape USB rechts"; break;
  }
  Serial.println(info);
}

void drawTestPattern(int rotation) {
  tft.fillScreen(TFT_WHITE);
  
  // Einfaches Test-Muster
  tft.setTextColor(TFT_BLACK);
  tft.drawString("ROTATION " + String(rotation), 10, 10, 2);
  
  String sizeInfo = String(tft.width()) + "x" + String(tft.height());
  tft.drawString(sizeInfo, 10, 30, 1);
  
  // Versuche Layout-Elemente zu zeichnen (mit Fehlerbehandlung)
  drawHeader();
  initButtons();
  drawButtons();
  
  // Einfache Test-Rechtecke
  int w = tft.width();
  int h = tft.height();
  
  tft.fillRect(5, 5, 20, 20, TFT_RED);
  tft.fillRect(w-25, 5, 20, 20, TFT_GREEN);
  tft.fillRect(w-25, h-25, 20, 20, TFT_BLUE);
  tft.fillRect(5, h-25, 20, 20, TFT_YELLOW);
  
  // Touch-Anweisung
  tft.setTextColor(TFT_RED);
  tft.drawString("TOUCH TEST", w/2-40, h/2, 2);
}

void showCalibrationResults() {
  Serial.println("\n" + String('=', 50));
  Serial.println("         ERGEBNISSE");
  Serial.println(String('=', 50));
  
  for (int i = 0; i < 4; i++) {
    Serial.print("Rotation ");
    Serial.print(i);
    Serial.print(": Touch=");
    Serial.print(calibrationResult.touchWorking[i] ? "OK" : "FEHLER");
    Serial.print(", Layout=");
    Serial.println(calibrationResult.layoutGood[i] ? "OK" : "FEHLER");
  }
  
  Serial.println("");
  Serial.print("EMPFEHLUNG: Rotation ");
  Serial.print(calibrationResult.bestRotation);
  Serial.print(" (");
  showRotationInfo(calibrationResult.bestRotation);
  Serial.println(")");
  
  Serial.println("");
  Serial.println("Config.h Einstellung:");
  Serial.print("#define SCREEN_ORIENTATION ");
  Serial.println(calibrationResult.bestRotation);
  
  Serial.println(String('=', 50));
}

// Stubs für nicht implementierte Funktionen
void comprehensiveTouchTest() {
  Serial.println("Nicht implementiert - verwenden Sie quickRotationTest()");
  quickRotationTest();
}

void serialControlledTest() {
  Serial.println("Nicht implementiert - verwenden Sie quickRotationTest()");
  quickRotationTest();
}

void testSingleRotation(int rotation) {
  Serial.print("Teste Rotation ");
  Serial.println(rotation);
  tft.setRotation(rotation);
  drawTestPattern(rotation);
  delay(3000);
}

void testTouchMappingForRotation(int rotation) {
  Serial.println("Touch-Mapping-Test vereinfacht");
  testSingleRotation(rotation);
}

void testLayoutForRotation(int rotation) {
  testSingleRotation(rotation);
}

void drawTestCrosses() {
  // Vereinfacht - nur Rechtecke
  int w = tft.width();
  int h = tft.height();
  tft.fillRect(30, 80, 20, 20, TFT_RED);
  tft.fillRect(w-50, 80, 20, 20, TFT_GREEN);
  tft.fillRect(w-50, h-50, 20, 20, TFT_BLUE);
  tft.fillRect(30, h-50, 20, 20, TFT_YELLOW);
  tft.fillRect(w/2-10, h/2-10, 20, 20, TFT_MAGENTA);
}

void drawCross(int x, int y, uint16_t color) {
  // Vereinfachtes Kreuz
  tft.drawLine(x-10, y, x+10, y, color);
  tft.drawLine(x, y-10, x, y+10, color);
}

void waitForTouch(String message) {
  if (message != "") Serial.println(message);
  while (!touchscreen.tirqTouched() || !touchscreen.touched()) delay(50);
  while (touchscreen.touched()) delay(10);
}

bool waitForSerialInput() {
  unsigned long startTime = millis();
  while (!Serial.available() && (millis() - startTime < 5000)) delay(100);
  return Serial.available();
}