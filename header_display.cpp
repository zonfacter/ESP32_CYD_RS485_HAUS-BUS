// *** NEU: Erstelle neue Datei header_display.cpp ***

#include "header_display.h"
#include "service_manager.h"

// Simulierte Zeit (da keine RTC vorhanden)
TimeInfo currentTime = {14, 30, 0, 26, 5, 2025};  // 14:30:00, 26.05.2025
unsigned long lastTimeUpdate = 0;

void setupHeaderDisplay() {
  // Zeit initialisieren
  lastTimeUpdate = millis();
  
  #if DB_INFO == 1
    Serial.println("DEBUG: Header-Display initialisiert");
  #endif
}

void updateSimulatedTime() {
  // Simulierte Zeit aktualisieren (jede Sekunde)
  if (millis() - lastTimeUpdate >= 1000) {
    currentTime.second++;
    
    if (currentTime.second >= 60) {
      currentTime.second = 0;
      currentTime.minute++;
      
      if (currentTime.minute >= 60) {
        currentTime.minute = 0;
        currentTime.hour++;
        
        if (currentTime.hour >= 24) {
          currentTime.hour = 0;
          currentTime.day++;
          
          // Einfache Monats-Logik (31 Tage)
          if (currentTime.day > 31) {
            currentTime.day = 1;
            currentTime.month++;
            
            if (currentTime.month > 12) {
              currentTime.month = 1;
              currentTime.year++;
            }
          }
        }
      }
    }
    
    lastTimeUpdate = millis();
  }
}

String formatTime() {
  String timeStr = "";
  if (currentTime.hour < 10) timeStr += "0";
  timeStr += String(currentTime.hour);
  timeStr += ":";
  if (currentTime.minute < 10) timeStr += "0";
  timeStr += String(currentTime.minute);
  return timeStr;
}

String formatDate() {
  String dateStr = "";
  if (currentTime.day < 10) dateStr += "0";
  dateStr += String(currentTime.day);
  dateStr += ".";
  if (currentTime.month < 10) dateStr += "0";
  dateStr += String(currentTime.month);
  dateStr += ".";
  dateStr += String(currentTime.year);
  return dateStr;
}

void drawHeader() {
  // Header-Hintergrund (dunkelgrau)
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, TFT_DARKGREY);
  tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, TFT_BLACK);
  
  // Text-Einstellungen
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(1);
  
  // Zeit links (Position 2, 6)
  String timeStr = formatTime();
  tft.drawString(timeStr, 2, 6, 1);
  
  // Datum links-mitte (Position 45, 6)
  String dateStr = formatDate();
  tft.drawString(dateStr, 45, 6, 1);
  
  // Device ID rechts-mitte
  String deviceID = "ID:" + serviceManager.getDeviceID();
  int deviceIdWidth = deviceID.length() * 6;  // Ungefähre Breite
  int deviceIdX = SCREEN_WIDTH - SERVICE_ICON_SIZE - deviceIdWidth - 8;
  tft.drawString(deviceID, deviceIdX, 6, 1);
  
  // Service-Icon rechts
  drawServiceIcon(serviceManager.isServiceMode());
}

void updateHeaderTime() {
  // Nur Zeit und Datum aktualisieren (ohne kompletten Header neu zu zeichnen)
  updateSimulatedTime();
  
  // Zeit-Bereich überschreiben
  tft.fillRect(2, 1, 40, 18, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  String timeStr = formatTime();
  tft.drawString(timeStr, 2, 6, 1);
  
  // Datum-Bereich überschreiben  
  tft.fillRect(45, 1, 70, 18, TFT_DARKGREY);
  String dateStr = formatDate();
  tft.drawString(dateStr, 45, 6, 1);
}

void drawServiceIcon(bool active) {
  // Service-Icon: Zahnrad-Symbol in 18x18 Pixel
  uint16_t iconColor = active ? TFT_YELLOW : TFT_LIGHTGREY;
  uint16_t bgColor = TFT_DARKGREY;
  
  int centerX = SERVICE_ICON_X + SERVICE_ICON_SIZE/2;
  int centerY = SERVICE_ICON_Y + SERVICE_ICON_SIZE/2;
  
  // Hintergrund löschen
  tft.fillRect(SERVICE_ICON_X, SERVICE_ICON_Y, SERVICE_ICON_SIZE, SERVICE_ICON_SIZE, bgColor);
  
  // Einfaches Zahnrad-Symbol (8x8 Pixel Kern + Zähne)
  // Äußerer Kreis (Zahnrad-Rand)
  tft.drawCircle(centerX, centerY, 8, iconColor);
  tft.drawCircle(centerX, centerY, 7, iconColor);
  
  // Innerer Kreis (Zahnrad-Mitte)
  tft.fillCircle(centerX, centerY, 3, iconColor);
  tft.fillCircle(centerX, centerY, 2, bgColor);
  
  // Zahnrad-Zähne (8 kleine Rechtecke)
  for (int i = 0; i < 8; i++) {
    float angle = i * 45.0 * PI / 180.0;
    int x = centerX + cos(angle) * 9;
    int y = centerY + sin(angle) * 9;
    tft.fillRect(x-1, y-1, 2, 2, iconColor);
  }
  
  // Service-Symbol "S" in der Mitte (falls gewünscht)
  tft.setTextColor(bgColor, iconColor);
  tft.drawString("S", centerX-3, centerY-4, 1);
}

bool checkServiceIconTouch(int x, int y) {
  // *** VERGRÖSSERTER Touch-Bereich: Komplette rechte obere Ecke ***
  bool inTouchArea = (x >= SERVICE_TOUCH_X && x <= SCREEN_WIDTH &&
                      y >= SERVICE_TOUCH_Y && y <= SERVICE_TOUCH_Y + SERVICE_TOUCH_AREA_HEIGHT);
  
  #if DB_INFO == 1
    if (inTouchArea) {
      Serial.print("DEBUG: Service-Icon Touch erkannt - X: ");
      Serial.print(x);
      Serial.print(", Y: ");
      Serial.print(y);
      Serial.print(" (Touch-Bereich: X=");
      Serial.print(SERVICE_TOUCH_X);
      Serial.print("-");
      Serial.print(SCREEN_WIDTH);
      Serial.print(", Y=");
      Serial.print(SERVICE_TOUCH_Y);
      Serial.print("-");
      Serial.print(SERVICE_TOUCH_Y + SERVICE_TOUCH_AREA_HEIGHT);
      Serial.println(")");
    }
  #endif
  
  return inTouchArea;
}

// *** NEU: Zeit/Datum Telegramm-Verarbeitung ***

bool parseTimeString(String timeStr, int &hour, int &minute, int &second) {
  // Format: HHMMSS (6 Zeichen)
  if (timeStr.length() != 6) return false;
  
  hour = timeStr.substring(0, 2).toInt();
  minute = timeStr.substring(2, 4).toInt(); 
  second = timeStr.substring(4, 6).toInt();
  
  // Validierung
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;
  
  return true;
}

bool parseDateString(String dateStr, int &day, int &month, int &year) {
  // Format: DDMMYYYY (8 Zeichen)
  if (dateStr.length() != 8) return false;
  
  day = dateStr.substring(0, 2).toInt();
  month = dateStr.substring(2, 4).toInt();
  year = dateStr.substring(4, 8).toInt();
  
  // Validierung
  if (day < 1 || day > 31) return false;
  if (month < 1 || month > 12) return false;
  if (year < 2020 || year > 2099) return false;
  
  return true;
}

void handleTimeSetTelegram(String params) {
  int hour, minute, second;
  
  if (parseTimeString(params, hour, minute, second)) {
    currentTime.hour = hour;
    currentTime.minute = minute;
    currentTime.second = second;
    lastTimeUpdate = millis();  // Reset Timer
    
    #if DB_RX_INFO == 1
      Serial.print("DEBUG: Zeit gesetzt auf: ");
      Serial.print(hour);
      Serial.print(":");
      Serial.print(minute);
      Serial.print(":");
      Serial.println(second);
    #endif
    
    // Header aktualisieren
    updateHeaderTime();
  } else {
    #if DB_RX_INFO == 1
      Serial.print("ERROR: Ungültiges Zeit-Format: ");
      Serial.print(params);
      Serial.println(" (erwartet: HHMMSS)");
    #endif
  }
}

void handleDateSetTelegram(String params) {
  int day, month, year;
  
  if (parseDateString(params, day, month, year)) {
    currentTime.day = day;
    currentTime.month = month;
    currentTime.year = year;
    
    #if DB_RX_INFO == 1
      Serial.print("DEBUG: Datum gesetzt auf: ");
      Serial.print(day);
      Serial.print(".");
      Serial.print(month);
      Serial.print(".");
      Serial.println(year);
    #endif
    
    // Header aktualisieren
    updateHeaderTime();
  } else {
    #if DB_RX_INFO == 1
      Serial.print("ERROR: Ungültiges Datum-Format: ");
      Serial.print(params);
      Serial.println(" (erwartet: DDMMYYYY)");
    #endif
  }
}