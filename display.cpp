// display.cpp
#include "display.h"

// TFT-Display-Objekt
TFT_eSPI tft = TFT_eSPI();

void setupDisplay() {
  tft.init();
  
  // Setze Rotation basierend auf der Bildschirmausrichtung
  #if SCREEN_ORIENTATION == PORTRAIT
    tft.setRotation(0);
  #else // LANDSCAPE
    tft.setRotation(1);
  #endif
}