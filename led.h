// led.h
#ifndef LED_H
#define LED_H

#include "config.h"

// Initialisiert die RGB-LED
void setupLed();

// Schaltet alle LED-Farben aus
void ledOff();

// Schaltet die rote LED ein (für das Senden)
void ledSendSignal();

// Schaltet die blaue LED ein (für den Empfang)
void ledReceiveSignal();

// Aktualisiert den LED-Status basierend auf Timern
// Diese Funktion sollte regelmäßig in der loop() aufgerufen werden
void updateLedStatus();

#endif // LED_H