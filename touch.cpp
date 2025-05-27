#include "touch.h"
#include "menu.h"
#include "backlight.h"
#include "communication.h"

// Touchscreen-Objekt
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Speichervariablen für Invertierung
bool invertTouchX = true;  // Standardmäßig invertiert
bool invertTouchY = false;  // Standardmäßig invertiert

// Initialisiert den Touchscreen
void setupTouch() {
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  
  // Setze Touchscreen-Rotation basierend auf der Bildschirmausrichtung
  #if SCREEN_ORIENTATION == PORTRAIT
    touchscreen.setRotation(0);
  #else // LANDSCAPE
    touchscreen.setRotation(1);
  #endif
}

// Touchscreen-Koordinaten zu kalibrierten Display-Koordinaten konvertieren
void getTouchPoint(int *x, int *y) {
  TS_Point p = touchscreen.getPoint();
  int rotation = tft.getRotation();

  switch (rotation) {
    case 0: // Portrait
      *x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, tft.width());
      *y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, tft.height());
      break;
      
    case 1: // Landscape 90° (USB links)
      *x = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, tft.width());
      // *** HIER: Y-Achse invertiert ***
      *y = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, tft.height());
      break;
      
    case 2: // Portrait 180°
      *x = map(p.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, tft.width());
      *y = map(p.y, TOUCH_MAX_Y, TOUCH_MIN_Y, 0, tft.height());
      break;
      
    case 3: // Landscape 270° (USB rechts)
      *x = map(p.y, TOUCH_MAX_Y, TOUCH_MIN_Y, 0, tft.width());
      // *** HIER: Y-Achse invertiert ***  
      *y = map(p.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, tft.height());
      break;
  }

  *x = constrain(*x, 0, tft.width() - 1);
  *y = constrain(*y, 0, tft.height() - 1);
}


// Zeichnet ein Kalibrierungskreuz
void drawCalibrationPoint(int x, int y, uint16_t color) {
  // Größeres und besser sichtbares Kreuz zeichnen
  tft.drawLine(x - 15, y, x + 15, y, color);
  tft.drawLine(x, y - 15, x, y + 15, color);
  tft.drawCircle(x, y, 8, color);
  tft.drawCircle(x, y, 9, color);
  
  // Positionsnummer neben dem Kreuz anzeigen
  int pointNum = 0;
  
  if (x < SCREEN_WIDTH / 2) {
    if (y < SCREEN_HEIGHT / 2) {
      pointNum = 1; // oben links
    } else {
      pointNum = 4; // unten links
    }
  } else {
    if (y < SCREEN_HEIGHT / 2) {
      pointNum = 2; // oben rechts
    } else if (x > 3*SCREEN_WIDTH/4 && y > 3*SCREEN_HEIGHT/4) {
      pointNum = 3; // unten rechts
    } else {
      pointNum = 5; // mitte
    }
  }
  
  if (pointNum > 0) {
    String numStr = String(pointNum);
    // Position des Textes abhängig von der Position des Kreuzes
    int textX, textY;
    
    if (x < SCREEN_WIDTH / 2) {
      textX = x + 25; // Rechts vom Kreuz
    } else {
      textX = x - 25; // Links vom Kreuz
    }
    
    if (y < SCREEN_HEIGHT / 2) {
      textY = y + 15; // Unter dem Kreuz
    } else {
      textY = y - 25; // Über dem Kreuz
    }
    
    tft.drawString(numStr, textX, textY, 2);
  }
}

// Testfunktion für die Kalibrierung der Touch-Koordinaten
void testTouch() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Touch-Kalibrierungstest", SCREEN_WIDTH/2, 10, 2);
  tft.drawCentreString("Bitte tippen Sie auf die Kreuze", SCREEN_WIDTH/2, 30, 2);
  
  // Zeichne Testkreuze an den vier Ecken und in der Mitte
  int padding = 30;
  
  // Zeichne Kreuze
  drawCalibrationPoint(padding, padding, TFT_RED);                           // oben links
  drawCalibrationPoint(SCREEN_WIDTH - padding, padding, TFT_GREEN);          // oben rechts
  drawCalibrationPoint(SCREEN_WIDTH - padding, SCREEN_HEIGHT - padding, TFT_BLUE); // unten rechts
  drawCalibrationPoint(padding, SCREEN_HEIGHT - padding, TFT_YELLOW);        // unten links
  drawCalibrationPoint(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, TFT_MAGENTA);        // mitte
  
  // Zusätzliche Buttons für Aktionen
  tft.fillRect(SCREEN_WIDTH - 60, 5, 55, 25, TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("ESC", SCREEN_WIDTH - 32, 12, 1);
  
  tft.fillRect(SCREEN_WIDTH - 130, 5, 60, 25, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("INV-X", SCREEN_WIDTH - 100, 12, 1);
  
  tft.fillRect(SCREEN_WIDTH - 130, 35, 60, 25, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("INV-Y", SCREEN_WIDTH - 100, 42, 1);
  
  // Hintergrundbeleuchtungstest-Buttons
  tft.fillRect(5, 5, 40, 25, TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("BL+", 25, 12, 1);
  
  tft.fillRect(50, 5, 40, 25, TFT_MAROON);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("BL-", 70, 12, 1);
  
  delay(500);
  
  bool testMode = true;
  int lastX = -1, lastY = -1;
  
  // Anzeige der aktuellen Kalibrierungswerte
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString("InvertX: " + String(invertTouchX ? "JA" : "NEIN"), 10, SCREEN_HEIGHT - 70, 1);
  tft.drawString("InvertY: " + String(invertTouchY ? "JA" : "NEIN"), 10, SCREEN_HEIGHT - 55, 1);
  tft.drawString("Backlight: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 40, 1);
  
  while (testMode) {
    // Prüfen, ob ein Statusupdate für die Hintergrundbeleuchtung fällig ist
    unsigned long currentMillis = millis();
    // *** KORRIGIERT: Konstante direkt verwenden ***
    if (currentMillis - lastBacklightStatusTime >= 23000) {  // BACKLIGHT_STATUS_INTERVAL = 23000
      sendBacklightStatus();
      lastBacklightStatusTime = currentMillis;
    }
    
    if (touchscreen.tirqTouched() && touchscreen.touched()) {
      TS_Point p = touchscreen.getPoint();
      int x, y;
      
      // Manuelle Kalibrierung zum Testen verschiedener Konfigurationen
      #if SCREEN_ORIENTATION == LANDSCAPE
        // Für Querformat
        if (invertTouchX) {
          x = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_WIDTH);
        } else {
          x = map(p.y, TOUCH_MAX_Y, TOUCH_MIN_Y, 0, SCREEN_WIDTH);
        }
        
        if (invertTouchY) {
          y = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_HEIGHT);
        } else {
          y = map(p.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, SCREEN_HEIGHT);
        }
      #else
        // Für Hochformat
        if (invertTouchX) {
          x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_WIDTH);
        } else {
          x = map(p.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, SCREEN_WIDTH);
        }
        
        if (invertTouchY) {
          y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_HEIGHT);
        } else {
          y = map(p.y, TOUCH_MAX_Y, TOUCH_MIN_Y, 0, SCREEN_HEIGHT);
        }
      #endif
      
      // Begrenze Koordinaten
      x = constrain(x, 0, SCREEN_WIDTH - 1);
      y = constrain(y, 0, SCREEN_HEIGHT - 1);
      
      // Überprüfen, ob ESC gedrückt wurde
      if (x >= SCREEN_WIDTH - 60 && x <= SCREEN_WIDTH - 5 && y >= 5 && y <= 30) {
        testMode = false;
        break;
      }
      
      // Überprüfen, ob INV-X gedrückt wurde
      if (x >= SCREEN_WIDTH - 130 && x <= SCREEN_WIDTH - 70 && y >= 5 && y <= 30) {
        invertTouchX = !invertTouchX;
        // Aktualisiere Anzeigetext
        tft.fillRect(0, SCREEN_HEIGHT - 70, 150, 40, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString("InvertX: " + String(invertTouchX ? "JA" : "NEIN"), 10, SCREEN_HEIGHT - 70, 1);
        tft.drawString("InvertY: " + String(invertTouchY ? "JA" : "NEIN"), 10, SCREEN_HEIGHT - 55, 1);
        tft.drawString("Backlight: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 40, 1);
        
        delay(200);
        continue;
      }
      
      // Überprüfen, ob INV-Y gedrückt wurde
      if (x >= SCREEN_WIDTH - 130 && x <= SCREEN_WIDTH - 70 && y >= 35 && y <= 60) {
        invertTouchY = !invertTouchY;
        // Aktualisiere Anzeigetext
        tft.fillRect(0, SCREEN_HEIGHT - 70, 150, 40, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString("InvertX: " + String(invertTouchX ? "JA" : "NEIN"), 10, SCREEN_HEIGHT - 70, 1);
        tft.drawString("InvertY: " + String(invertTouchY ? "JA" : "NEIN"), 10, SCREEN_HEIGHT - 55, 1);
        tft.drawString("Backlight: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 40, 1);
        
        delay(200);
        continue;
      }
      
      // Überprüfen, ob BL+ (Hintergrundbeleuchtung erhöhen) gedrückt wurde
      if (x >= 5 && x <= 45 && y >= 5 && y <= 30) {
        // Erhöhe Hintergrundbeleuchtung um 10%
        setBacklight(currentBacklight + 10);
        // Aktualisiere Anzeigetext
        tft.fillRect(0, SCREEN_HEIGHT - 40, 150, 20, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString("Backlight: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 40, 1);
        
        sendBacklightStatus();
        delay(200);
        continue;
      }
      
      // Überprüfen, ob BL- (Hintergrundbeleuchtung verringern) gedrückt wurde
      if (x >= 50 && x <= 90 && y >= 5 && y <= 30) {
        // Verringere Hintergrundbeleuchtung um 10%
        setBacklight(currentBacklight - 10);
        // Aktualisiere Anzeigetext
        tft.fillRect(0, SCREEN_HEIGHT - 40, 150, 20, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString("Backlight: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 40, 1);
        
        sendBacklightStatus();
        delay(200);
        continue;
      }
      
      // Nur zeichnen, wenn sich die Position geändert hat
      if (x != lastX || y != lastY) {
        // Position ausgeben
        tft.fillRect(10, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 20, 20, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        String positionText = "Touch: X=" + String(x) + " Y=" + String(y);
        tft.drawCentreString(positionText, SCREEN_WIDTH/2, SCREEN_HEIGHT - 20, 1);
        
        // Zeige Rohwerte an
        tft.fillRect(150, SCREEN_HEIGHT - 70, SCREEN_WIDTH - 160, 40, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString("Raw X: " + String(p.x), 150, SCREEN_HEIGHT - 70, 1);
        tft.drawString("Raw Y: " + String(p.y), 150, SCREEN_HEIGHT - 55, 1);
        
        // Position markieren
        tft.fillCircle(x, y, 5, TFT_RED);
        
        lastX = x;
        lastY = y;
      }
      
      delay(50);
    }
  }
  
  // Die aktuellen Einstellungen im seriellen Monitor anzeigen
  Serial.println("Aktuelle Touch-Kalibrierung:");
  Serial.print("Invert X: ");
  Serial.println(invertTouchX ? "JA" : "NEIN");
  Serial.print("Invert Y: ");
  Serial.println(invertTouchY ? "JA" : "NEIN");
  
  // Zurück zum Menü
  showMenu();
}

void touchCalibrationWizard() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("TOUCH-KALIBRIERUNG", SCREEN_WIDTH/2, 10, 2);
  tft.drawCentreString("ESP32-CYD Landscape Mode", SCREEN_WIDTH/2, 30, 1);
  
  // Zeige aktuelle Rotation
  tft.drawString("Rotation: " + String(tft.getRotation()), 10, 50, 1);
  tft.drawString("Screen: " + String(tft.width()) + "x" + String(tft.height()), 10, 65, 1);
  
  // Test-Kreuze an allen vier Ecken + Mitte
  struct TestPoint {
    int x, y;
    String label;
    uint16_t color;
  };
  
  TestPoint testPoints[] = {
    {30, 30, "1: Oben Links", TFT_RED},
    {SCREEN_WIDTH-30, 30, "2: Oben Rechts", TFT_GREEN},  
    {SCREEN_WIDTH-30, SCREEN_HEIGHT-30, "3: Unten Rechts", TFT_BLUE},
    {30, SCREEN_HEIGHT-30, "4: Unten Links", TFT_YELLOW},
    {SCREEN_WIDTH/2, SCREEN_HEIGHT/2, "5: Mitte", TFT_MAGENTA}
  };
  
  // Zeichne Test-Kreuze
  for (int i = 0; i < 5; i++) {
    TestPoint& tp = testPoints[i];
    
    // Kreuz zeichnen
    tft.drawLine(tp.x-15, tp.y, tp.x+15, tp.y, tp.color);
    tft.drawLine(tp.x, tp.y-15, tp.x, tp.y+15, tp.color);
    tft.drawCircle(tp.x, tp.y, 10, tp.color);
    
    // Label
    tft.setTextColor(tp.color);
    tft.drawString(tp.label, tp.x+20, tp.y-10, 1);
  }
  
  // Buttons für verschiedene Touch-Modi
  tft.fillRect(10, 85, 80, 30, TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("MODUS 1", 50, 95, 1);
  
  tft.fillRect(100, 85, 80, 30, TFT_NAVY);
  tft.drawCentreString("MODUS 2", 140, 95, 1);
  
  tft.fillRect(190, 85, 80, 30, TFT_MAROON);
  tft.drawCentreString("MODUS 3", 230, 95, 1);
  
  tft.fillRect(SCREEN_WIDTH-60, 5, 55, 25, TFT_RED);
  tft.drawCentreString("EXIT", SCREEN_WIDTH-32, 12, 1);
  
  // Touch-Modus-Variablen
  int touchMode = 1;
  bool testActive = true;
  
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString("Aktueller Modus: " + String(touchMode), 10, 120, 1);
  tft.drawString("Berühren Sie die Kreuze zum Testen", 10, 140, 1);
  
  while (testActive) {
    if (touchscreen.tirqTouched() && touchscreen.touched()) {
      TS_Point rawTouch = touchscreen.getPoint();
      int mappedX, mappedY;
      
      // Verschiedene Touch-Modi testen
      switch (touchMode) {
        case 1: // Standard ESP32-CYD Landscape
          mappedX = map(rawTouch.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, tft.width());
          mappedY = map(rawTouch.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, tft.height());
          break;
          
        case 2: // Alternative Mapping
          mappedX = map(rawTouch.y, TOUCH_MAX_Y, TOUCH_MIN_Y, 0, tft.width());
          mappedY = map(rawTouch.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, tft.height());
          break;
          
        case 3: // Invertiert
          mappedX = map(rawTouch.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, tft.width());
          mappedY = map(rawTouch.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, tft.height());
          break;
      }
      
      // Koordinaten begrenzen
      mappedX = constrain(mappedX, 0, tft.width() - 1);
      mappedY = constrain(mappedY, 0, tft.height() - 1);
      
      // Prüfe Button-Bereiche
      if (mappedY >= 85 && mappedY <= 115) {
        if (mappedX >= 10 && mappedX <= 90) {
          touchMode = 1;
        } else if (mappedX >= 100 && mappedX <= 180) {
          touchMode = 2;
        } else if (mappedX >= 190 && mappedX <= 270) {
          touchMode = 3;
        }
        
        // Update Modus-Anzeige
        tft.fillRect(10, 120, 200, 15, TFT_WHITE);
        tft.setTextColor(TFT_BLACK);
        tft.drawString("Aktueller Modus: " + String(touchMode), 10, 120, 1);
      }
      
      // Exit-Button
      if (mappedX >= SCREEN_WIDTH-60 && mappedY >= 5 && mappedY <= 30) {
        testActive = false;
        break;
      }
      
      // Zeige Touch-Position
      tft.fillRect(10, 160, SCREEN_WIDTH-20, 60, TFT_WHITE);
      tft.setTextColor(TFT_BLACK);
      tft.drawString("Raw: X=" + String(rawTouch.x) + " Y=" + String(rawTouch.y), 10, 165, 1);
      tft.drawString("Mapped: X=" + String(mappedX) + " Y=" + String(mappedY), 10, 180, 1);
      
      // Bestimme nächstes Test-Kreuz
      int nearestPoint = -1;
      int minDistance = 999999;
      for (int i = 0; i < 5; i++) {
        int distance = sqrt(pow(mappedX - testPoints[i].x, 2) + pow(mappedY - testPoints[i].y, 2));
        if (distance < minDistance) {
          minDistance = distance;
          nearestPoint = i;
        }
      }
      
      if (nearestPoint >= 0 && minDistance < 50) {
        tft.setTextColor(testPoints[nearestPoint].color);
        tft.drawString("Nächstes Kreuz: " + testPoints[nearestPoint].label, 10, 195, 1);
        tft.drawString("Entfernung: " + String(minDistance) + " Pixel", 10, 210, 1);
      } else {
        tft.setTextColor(TFT_RED);
        tft.drawString("Kein Kreuz in der Nähe", 10, 195, 1);
      }
      
      // Visueller Touch-Punkt
      tft.fillCircle(mappedX, mappedY, 3, TFT_BLACK);
      
      delay(100);
      
      // Warten bis Touch losgelassen
      while (touchscreen.touched()) {
        delay(10);
      }
    }
    delay(50);
  }
  
  // Zeige Empfehlung
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("KALIBRIERUNG ABGESCHLOSSEN", SCREEN_WIDTH/2, 50, 2);
  tft.drawCentreString("Bester Modus war: " + String(touchMode), SCREEN_WIDTH/2, 80, 2);
  
  String recommendation;
  switch (touchMode) {
    case 1: 
      recommendation = "Verwenden Sie Standard ESP32-CYD Mapping";
      break;
    case 2: 
      recommendation = "Verwenden Sie invertiertes Y-Mapping";
      break;
    case 3: 
      recommendation = "Verwenden Sie direktes X/Y-Mapping";
      break;
  }
  
  tft.drawCentreString(recommendation, SCREEN_WIDTH/2, 110, 1);
  tft.drawCentreString("Touch zum Fortfahren...", SCREEN_WIDTH/2, 140, 1);
  
  // Warten auf Touch
  while (!touchscreen.tirqTouched() || !touchscreen.touched()) {
    delay(100);
  }
  
  // Zurück zum normalen Betrieb
  showMenu();
}