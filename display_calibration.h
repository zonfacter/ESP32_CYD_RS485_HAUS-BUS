/**
 * display_calibration.h - Display Inbetriebnahme & Kalibrierung
 * 
 * Separate Test-Funktionen für neue Displays:
 * - Alle 4 Rotationen testen
 * - Touch-Kalibrierung für jede Rotation
 * - Layout-Verifikation
 * - Touch-Mapping-Test
 * 
 * Verwendung:
 * 1. #include "display_calibration.h" in Hauptdatei
 * 2. startDisplayCalibration() in setup() aufrufen
 * 3. Nach Kalibrierung: Include entfernen
 */

#ifndef DISPLAY_CALIBRATION_H
// Kommentierung löschen wir Display Orientierung - Touch Test - grob integriert.
//#define DISPLAY_CALIBRATION_H

#include "config.h"

// *** HAUPT-KALIBRIERUNGS-FUNKTIONEN ***
void startDisplayCalibration();           // Hauptfunktion - startet alle Tests
void quickRotationTest();                // Schneller Test aller 4 Rotationen
void comprehensiveTouchTest();           // Ausführlicher Touch-Test
void serialControlledTest();             // Test über Serial Monitor

// *** EINZELNE TEST-FUNKTIONEN ***
void testSingleRotation(int rotation);           // Eine Rotation testen
void testTouchMappingForRotation(int rotation);  // Touch-Mapping einer Rotation
void testLayoutForRotation(int rotation);        // Layout einer Rotation
void showRotationInfo(int rotation);             // Info über Rotation anzeigen

// *** HILFSFUNKTIONEN ***
void drawTestPattern(int rotation);              // Test-Muster zeichnen
void drawTestCrosses();                          // Touch-Test-Kreuze
void drawCross(int x, int y, uint16_t color);    // Einzelnes Kreuz
void waitForTouch(String message = "");          // Warten auf Touch
bool waitForSerialInput();                       // Warten auf Serial-Eingabe
void showCalibrationResults();                   // Ergebnisse anzeigen

// *** KONFIGURATION ***
#define CALIBRATION_ENABLED true                 // false = Kalibrierung deaktiviert
#define AUTO_START_CALIBRATION false            // true = automatisch starten
#define QUICK_TEST_DURATION 3000                // 3 Sekunden pro Rotation
#define TOUCH_TEST_TIMEOUT 30000                // 30 Sekunden Touch-Test

// *** ERGEBNIS-STRUKTUR ***
struct CalibrationResult {
  int bestRotation;
  bool touchWorking[4];      // Für jede Rotation
  bool layoutGood[4];        // Layout OK?
  String recommendation;     // Empfehlung
};

extern CalibrationResult calibrationResult;

#endif // DISPLAY_CALIBRATION_H