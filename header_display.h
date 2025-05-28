/**
 * header_display.h - Version 1.60 - KORRIGIERT für alle Rotationen
 * 
 * Header-Display System für ESP32 Touch-Interface
 * - 20px Header mit Zeit, Datum, Device ID, Service-Icon
 * - Automatische Positionierung je nach TFT-Rotation
 * - Telegramm-Steuerung für Zeit/Datum
 * - Service-Icon Touch-Aktivierung
 */

#ifndef HEADER_DISPLAY_H
#define HEADER_DISPLAY_H

#include "config.h"

// Header-Konstanten
#define HEADER_HEIGHT 20
#define SERVICE_ICON_SIZE 18

#define SERVICE_TOUCH_AREA_WIDTH 60   
#define SERVICE_TOUCH_AREA_HEIGHT 30  

// Header-Funktionen
void setupHeaderDisplay();
void drawHeader();
void updateHeaderTime();
void drawServiceIcon(bool active = false);
bool checkServiceIconTouch(int x, int y);

// *** NEU: Positionierungs-Hilfsfunktionen ***
int getHeaderY();                    // Header Y-Position je nach Rotation
int getButtonAreaY();               // Button-Bereich Y-Position  
int getAvailableButtonHeight();     // Verfügbare Höhe für Buttons
int getHeaderOffset();              // Header-Offset für menu.cpp

// Zeit-Funktionen (simuliert - da keine RTC)
struct TimeInfo {
  int hour;
  int minute;
  int second;
  int day;
  int month;
  int year;
};

extern TimeInfo currentTime;
void updateSimulatedTime();
String formatTime();
String formatDate();

// Zeit/Datum Telegramm-Verarbeitung
bool parseTimeString(String timeStr, int &hour, int &minute, int &second);
bool parseDateString(String dateStr, int &day, int &month, int &year);
void handleTimeSetTelegram(String params);
void handleDateSetTelegram(String params);

#endif // HEADER_DISPLAY_H