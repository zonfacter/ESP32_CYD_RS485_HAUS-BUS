// *** KORRIGIERTE header_display.cpp für alle TFT-Rotationen ***

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

// *** NEU: Header-Position basierend auf TFT-Rotation berechnen ***
int getHeaderY() {
  int rotation = tft.getRotation();
  
  switch (rotation) {
    case 0:  // Portrait USB oben - Header soll physisch oben sein
      return tft.height() - HEADER_HEIGHT;  // Unten im Koordinatensystem = oben physisch
    case 1:  // Landscape USB links - Header soll physisch oben sein  
      return 0;  // Oben im Koordinatensystem = oben physisch
    case 2:  // Portrait USB unten - Header soll physisch oben sein
      return 0;  // Oben im Koordinatensystem = oben physisch
    case 3:  // Landscape USB rechts - Header soll physisch oben sein
      return 0;  // Oben im Koordinatensystem = oben physisch
    default:
      return 0;
  }
}

// *** NEU: Button-Bereich Y-Position berechnen ***
int getButtonAreaY() {
  int rotation = tft.getRotation();
  
  switch (rotation) {
    case 0:  // Portrait USB oben
      return 0;  // Buttons oben im Koordinatensystem = unten physisch (unter Header)
    case 1:  // Landscape USB links
      return HEADER_HEIGHT + 5;  // Buttons unter Header
    case 2:  // Portrait USB unten  
      return HEADER_HEIGHT + 5;  // Buttons unter Header
    case 3:  // Landscape USB rechts
      return HEADER_HEIGHT + 5;  // Buttons unter Header
    default:
      return HEADER_HEIGHT + 5;
  }
}

// *** NEU: Verfügbare Button-Höhe berechnen ***
int getAvailableButtonHeight() {
  int rotation = tft.getRotation();
  int screenHeight = tft.height();
  
  switch (rotation) {
    case 0:  // Portrait USB oben - Header physisch oben
      return screenHeight - HEADER_HEIGHT - 5;  // Volle Höhe minus Header
    default:  // Alle anderen Rotationen
      return screenHeight - HEADER_HEIGHT - 5;
  }
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
  // *** KORRIGIERTE Header-Position ***
  int currentScreenWidth = tft.width();
  int currentScreenHeight = tft.height(); 
  int headerY = getHeaderY();
  int rotation = tft.getRotation();
  
  #if DB_INFO == 1
    Serial.print("DEBUG: drawHeader() - Rotation: ");
    Serial.print(rotation);
    Serial.print(", Screen: ");
    Serial.print(currentScreenWidth);
    Serial.print("x");
    Serial.print(currentScreenHeight);
    Serial.print(", Header Y-Position: ");
    Serial.println(headerY);
  #endif
  
  // Header-Hintergrund (dunkelgrau)
  tft.fillRect(0, headerY, currentScreenWidth, HEADER_HEIGHT, TFT_DARKGREY);
  
  // Header-Linie (unter oder über dem Header je nach Position)
  if (headerY == 0) {
    // Header oben → Linie unten
    tft.drawLine(0, headerY + HEADER_HEIGHT, currentScreenWidth, headerY + HEADER_HEIGHT, TFT_BLACK);
  } else {
    // Header unten → Linie oben  
    tft.drawLine(0, headerY, currentScreenWidth, headerY, TFT_BLACK);
  }
  
  // Text-Einstellungen
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(1);
  
  // *** KORRIGIERTE Text-Positionen ***
  int textY = headerY + 6;  // 6 Pixel vom Header-Rand nach innen
  
  // Zeit links (Position 2, textY)
  String timeStr = formatTime();
  tft.drawString(timeStr, 2, textY, 1);
  
  // Datum links-mitte (Position 45, textY)
  String dateStr = formatDate();
  tft.drawString(dateStr, 45, textY, 1);
  
  // Device ID rechts-mitte - DYNAMISCH berechnet
  String deviceID = "ID:" + serviceManager.getDeviceID();
  int deviceIdWidth = deviceID.length() * 6;  // Ungefähre Breite
  int deviceIdX = currentScreenWidth - SERVICE_ICON_SIZE - deviceIdWidth - 8;
  tft.drawString(deviceID, deviceIdX, textY, 1);
  
  // Service-Icon rechts - DYNAMISCH berechnet
  drawServiceIcon(serviceManager.isServiceMode());
}

void updateHeaderTime() {
  // *** KORRIGIERTE Zeit-Update mit dynamischer Y-Position ***
  int currentScreenWidth = tft.width();
  int headerY = getHeaderY();
  int textY = headerY + 6;
  
  // Nur Zeit und Datum aktualisieren (ohne kompletten Header neu zu zeichnen)
  updateSimulatedTime();
  
  // Zeit-Bereich überschreiben
  tft.fillRect(2, headerY + 1, 40, 18, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  String timeStr = formatTime();
  tft.drawString(timeStr, 2, textY, 1);
  
  // Datum-Bereich überschreiben  
  tft.fillRect(45, headerY + 1, 70, 18, TFT_DARKGREY);
  String dateStr = formatDate();
  tft.drawString(dateStr, 45, textY, 1);
  
  // *** Service-Icon bei Orientierungsumschaltung neu positionieren ***
  drawServiceIcon(serviceManager.isServiceMode());
}

void drawServiceIcon(bool active) {
  // *** KORRIGIERTE Service-Icon Position mit dynamischer Y-Position ***
  int currentScreenWidth = tft.width();
  int headerY = getHeaderY();
  int serviceIconX = currentScreenWidth - SERVICE_ICON_SIZE - 2;
  int serviceIconY = headerY + 1;  // 1 Pixel vom Header-Rand
  
  // Service-Icon: Zahnrad-Symbol in 18x18 Pixel
  uint16_t iconColor = active ? TFT_YELLOW : TFT_LIGHTGREY;
  uint16_t bgColor = TFT_DARKGREY;
  
  int centerX = serviceIconX + SERVICE_ICON_SIZE/2;
  int centerY = serviceIconY + SERVICE_ICON_SIZE/2;
  
  // Hintergrund löschen
  tft.fillRect(serviceIconX, serviceIconY, SERVICE_ICON_SIZE, SERVICE_ICON_SIZE, bgColor);
  
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
  
  // Service-Symbol "S" in der Mitte
  tft.setTextColor(bgColor, iconColor);
  tft.drawString("S", centerX-3, centerY-4, 1);
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Service-Icon gezeichnet bei X=");
    Serial.print(serviceIconX);
    Serial.print(", Y=");
    Serial.print(serviceIconY);
    Serial.print(" (Header Y=");
    Serial.print(headerY);
    Serial.print(", Screen: ");
    Serial.print(currentScreenWidth);
    Serial.print("x");
    Serial.print(tft.height());
    Serial.println(")");
  #endif
}

bool checkServiceIconTouch(int x, int y) {
  // *** KORRIGIERTE Touch-Bereich mit dynamischer Y-Position ***
  int currentScreenWidth = tft.width();
  int headerY = getHeaderY();
  int serviceTouchX = currentScreenWidth - SERVICE_TOUCH_AREA_WIDTH;
  int serviceTouchY = headerY;  // Touch-Bereich beginnt beim Header
  
  // *** VERGRÖSSERTER Touch-Bereich: Header-Höhe + etwas darüber/darunter ***
  bool inTouchArea = (x >= serviceTouchX && x <= currentScreenWidth &&
                      y >= serviceTouchY && y <= serviceTouchY + SERVICE_TOUCH_AREA_HEIGHT);
  
  #if DB_INFO == 1
    if (inTouchArea) {
      Serial.print("DEBUG: Service-Icon Touch erkannt - X: ");
      Serial.print(x);
      Serial.print(", Y: ");
      Serial.print(y);
      Serial.print(" (Touch-Bereich: X=");
      Serial.print(serviceTouchX);
      Serial.print("-");
      Serial.print(currentScreenWidth);
      Serial.print(", Y=");
      Serial.print(serviceTouchY);
      Serial.print("-");
      Serial.print(serviceTouchY + SERVICE_TOUCH_AREA_HEIGHT);
      Serial.print(", Header Y=");
      Serial.print(headerY);
      Serial.print(", Screen: ");
      Serial.print(currentScreenWidth);
      Serial.print("x");
      Serial.print(tft.height());
      Serial.println(")");
    }
  #endif
  
  return inTouchArea;
}

// *** NEU: Öffentliche Funktion für menu.cpp ***
int getHeaderOffset() {
  int rotation = tft.getRotation();
  
  switch (rotation) {
    case 0:  // Portrait USB oben - Header physisch oben, Buttons physisch unten
      return 5;  // Minimaler Offset da Buttons unten sind
    default:  // Alle anderen - Header oben, Buttons darunter
      return HEADER_HEIGHT + 5;
  }
}

// *** Bestehende Zeit/Datum-Funktionen bleiben unverändert ***

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