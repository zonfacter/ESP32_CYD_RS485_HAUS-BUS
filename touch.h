#ifndef TOUCH_H
#define TOUCH_H

#include "config.h"

// Initialisiert den Touchscreen
void setupTouch();

// Touchscreen-Koordinaten zu kalibrierten Display-Koordinaten konvertieren
void getTouchPoint(int *x, int *y);

// Testfunktion für die Kalibrierung der Touch-Koordinaten
void testTouch();

// Zeichnet ein Kalibrierungskreuz
void drawCalibrationPoint(int x, int y, uint16_t color);

// Touch Wizard
void touchCalibrationWizard();

#endif // TOUCH_H