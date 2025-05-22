// backlight.h
#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include "config.h"

// Initialisiere die Hintergrundbeleuchtung
void setupBacklight();

// PWM-Steuerung für die Hintergrundbeleuchtung (0-100%)
void setBacklight(int percent);

//#ifdef ESP32
// PWM-Implementierung (nur für ESP32)
void setBacklightPWM(int value);  // Funktion deklarieren
//#endif

// Einfache digitale Implementierung (Ein/Aus)
void setBacklightDigital(int percent);

// Sendet den aktuellen Status der Hintergrundbeleuchtung
void sendBacklightStatus();

// Extern deklariert, wird in anderen Dateien verwendet
extern unsigned long lastBacklightStatusTime;
extern int currentBacklight;

#endif // BACKLIGHT_H