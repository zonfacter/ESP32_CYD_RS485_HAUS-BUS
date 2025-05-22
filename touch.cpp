#include "touch.h"
#include "menu.h"
#include "backlight.h"
#include "communication.h"

// Touchscreen-Objekt
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Speichervariablen für Invertierung
bool invertTouchX = true;  // Standardmäßig invertiert
bool invertTouchY = true;  // Standardmäßig invertiert

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
  
  // XY-Achsen sind bei diesem Display vertauscht und möglicherweise gespiegelt
  // Konfigurierbare Werte für XPT2046 mit ST7789 im Querformat
  #if SCREEN_ORIENTATION == LANDSCAPE
    // Für Querformat: XY-Achsen tauschen und X spiegeln wenn nötig
    if (invertTouchX) {
      *x = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_WIDTH);
    } else {
      *x = map(p.y, TOUCH_MAX_Y, TOUCH_MIN_Y, 0, SCREEN_WIDTH);
    }
    
    if (invertTouchY) {
      *y = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_HEIGHT);
    } else {
      *y = map(p.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, SCREEN_HEIGHT);
    }
  #else
    // Für Hochformat
    if (invertTouchX) {
      *x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_WIDTH);
    } else {
      *x = map(p.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, SCREEN_WIDTH);
    }
    
    if (invertTouchY) {
      *y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_HEIGHT);
    } else {
      *y = map(p.y, TOUCH_MAX_Y, TOUCH_MIN_Y, 0, SCREEN_HEIGHT);
    }
  #endif
  
  // Begrenzen auf gültigen Bereich
  *x = constrain(*x, 0, SCREEN_WIDTH - 1);
  *y = constrain(*y, 0, SCREEN_HEIGHT - 1);
  
  // Protokollierung für Debugging-Zwecke
  #if DB_INFO == 1  // Allgemeine Informationen
  Serial.print("Raw: (");
  Serial.print(p.x);
  Serial.print(", ");
  Serial.print(p.y);
  Serial.print(") -> Mapped: (");
  Serial.print(*x);
  Serial.print(", ");
  Serial.print(*y);
  Serial.println(")");
  #endif
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
    if (currentMillis - lastBacklightStatusTime >= BACKLIGHT_STATUS_INTERVAL) {
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