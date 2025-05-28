// display.cpp
#include "config.h"

void setupDisplay() {
  tft.init();
  
  // *** KORRIGIERT: Direkt SCREEN_ORIENTATION verwenden ***
  tft.setRotation(SCREEN_ORIENTATION);
  
  #if DB_INFO == 1
    Serial.print("DEBUG: setupDisplay() - setRotation(");
    Serial.print(SCREEN_ORIENTATION);
    Serial.print(") → TFT Size: ");
    Serial.print(tft.width());
    Serial.print("x");
    Serial.println(tft.height());
  #endif
}