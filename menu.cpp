#include "menu.h"
#include "backlight.h"
#include "header_display.h"

// Button-Variablen
Button buttons[NUM_BUTTONS];
int buttonWidth, buttonHeight;

// Standard-Farben für Buttons
#define BUTTON_COLOR_INACTIVE TFT_DARKGREY    // Grau bei nicht Betätigung
#define BUTTON_COLOR_ACTIVE TFT_GREEN         // Grün wenn aktiv/betätigt

// Zeichnet alle Buttons
void drawButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    // Wähle die richtige Farbe basierend auf dem Button-Status
    uint16_t buttonColor = buttons[i].isActive ? BUTTON_COLOR_ACTIVE : BUTTON_COLOR_INACTIVE;
    
    tft.fillRect(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h, buttonColor);
    tft.drawRect(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h, TFT_BLACK);
    
    // Zentrieren des Textes
    int textWidth = buttons[i].label.length() * 6; // Ungefähre Breite
    int textX = buttons[i].x + (buttons[i].w - textWidth) / 2;
    int textY = buttons[i].y + (buttons[i].h - 8) / 2;
    
    tft.setTextColor(buttons[i].textColor);
    tft.drawString(buttons[i].label, textX, textY, 2);
  }
}

// Zeichnet einen einzelnen Button neu (für Statusaktualisierungen)
void redrawButton(int buttonIndex) {
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) return;
  
  // Wähle die richtige Farbe basierend auf dem Button-Status
  uint16_t buttonColor = buttons[buttonIndex].isActive ? BUTTON_COLOR_ACTIVE : BUTTON_COLOR_INACTIVE;
    
  tft.fillRect(buttons[buttonIndex].x, buttons[buttonIndex].y, 
               buttons[buttonIndex].w, buttons[buttonIndex].h, buttonColor);
  tft.drawRect(buttons[buttonIndex].x, buttons[buttonIndex].y, 
               buttons[buttonIndex].w, buttons[buttonIndex].h, TFT_BLACK);
  
  // Zentrieren des Textes
  int textWidth = buttons[buttonIndex].label.length() * 6; // Ungefähre Breite
  int textX = buttons[buttonIndex].x + (buttons[buttonIndex].w - textWidth) / 2;
  int textY = buttons[buttonIndex].y + (buttons[buttonIndex].h - 8) / 2;
  
  tft.setTextColor(buttons[buttonIndex].textColor);
  tft.drawString(buttons[buttonIndex].label, textX, textY, 2);
}

// Setzt den Aktivierungsstatus eines Buttons und zeichnet ihn neu
void setButtonActive(int buttonIndex, bool active) {
  if (buttonIndex >= 0 && buttonIndex < NUM_BUTTONS) {
    buttons[buttonIndex].isActive = active;
    
    if (active) {
      buttons[buttonIndex].color = BUTTON_COLOR_ACTIVE;   // Grün
    } else {
      buttons[buttonIndex].color = BUTTON_COLOR_INACTIVE; // Grau
    }
    
    // Button neu zeichnen
    redrawButton(buttonIndex);
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Button ");
      Serial.print(buttonIndex);
      Serial.print(" (");
      Serial.print(buttons[buttonIndex].label);
      Serial.print(") gesetzt auf: ");
      Serial.println(active ? "AKTIV (grün)" : "INAKTIV (grau)");
    #endif
  }
}

// Findet einen Button anhand seiner Instanz-ID
int findButtonByInstanceID(String instanceID) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttons[i].instanceID == instanceID) {
      return i;
    }
  }
  return -1; // Button nicht gefunden
}

// Prüft, ob ein Button gedrückt wurde
int checkButtonPress(int x, int y) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (x >= buttons[i].x && x <= (buttons[i].x + buttons[i].w) &&
        y >= buttons[i].y && y <= (buttons[i].y + buttons[i].h)) {
      return i;
    }
  }
  return -1; // Kein Button gedrückt
}

// Initialisiert die Buttons mit ihren Positionen und Farben
void initButtons() {
  // *** KORRIGIERT: Button-Layout mit Header-Offset, ohne Titel und Helligkeit ***
  int headerOffset = HEADER_HEIGHT + 5;  // Nur Header + kleiner Abstand
  int availableHeight = SCREEN_HEIGHT - headerOffset;  // Komplette verfügbare Höhe
  
  #if SCREEN_ORIENTATION == PORTRAIT
    // Portrait: 2 Spalten x 3 Zeilen
    buttonWidth = SCREEN_WIDTH / 2;
    buttonHeight = availableHeight / 3;  // Volle Höhe nutzen
    
    // Button 1 (oben links)
    buttons[0].x = 0;
    buttons[0].y = headerOffset;
    buttons[0].w = buttonWidth;
    buttons[0].h = buttonHeight;
    
    // Button 2 (oben rechts)
    buttons[1].x = buttonWidth;
    buttons[1].y = headerOffset;
    buttons[1].w = buttonWidth;
    buttons[1].h = buttonHeight;
    
    // Button 3 (mitte links)
    buttons[2].x = 0;
    buttons[2].y = headerOffset + buttonHeight;
    buttons[2].w = buttonWidth;
    buttons[2].h = buttonHeight;
    
    // Button 4 (mitte rechts)
    buttons[3].x = buttonWidth;
    buttons[3].y = headerOffset + buttonHeight;
    buttons[3].w = buttonWidth;
    buttons[3].h = buttonHeight;
    
    // Button 5 (unten links)
    buttons[4].x = 0;
    buttons[4].y = headerOffset + buttonHeight * 2;
    buttons[4].w = buttonWidth;
    buttons[4].h = buttonHeight;
    
    // Button 6 (unten rechts)
    buttons[5].x = buttonWidth;
    buttons[5].y = headerOffset + buttonHeight * 2;
    buttons[5].w = buttonWidth;
    buttons[5].h = buttonHeight;
  #else
    // Landscape: 3 Spalten x 2 Zeilen
    buttonWidth = SCREEN_WIDTH / 3;
    buttonHeight = availableHeight / 2;  // Volle Höhe nutzen
    
    // Button 1 (oben links)
    buttons[0].x = 0;
    buttons[0].y = headerOffset;
    buttons[0].w = buttonWidth;
    buttons[0].h = buttonHeight;
    
    // Button 2 (oben mitte)
    buttons[1].x = buttonWidth;
    buttons[1].y = headerOffset;
    buttons[1].w = buttonWidth;
    buttons[1].h = buttonHeight;
    
    // Button 3 (oben rechts)
    buttons[2].x = buttonWidth * 2;
    buttons[2].y = headerOffset;
    buttons[2].w = buttonWidth;
    buttons[2].h = buttonHeight;
    
    // Button 4 (unten links)
    buttons[3].x = 0;
    buttons[3].y = headerOffset + buttonHeight;
    buttons[3].w = buttonWidth;
    buttons[3].h = buttonHeight;
    
    // Button 5 (unten mitte)
    buttons[4].x = buttonWidth;
    buttons[4].y = headerOffset + buttonHeight;
    buttons[4].w = buttonWidth;
    buttons[4].h = buttonHeight;
    
    // Button 6 (unten rechts)
    buttons[5].x = buttonWidth * 2;
    buttons[5].y = headerOffset + buttonHeight;
    buttons[5].w = buttonWidth;
    buttons[5].h = buttonHeight;
  #endif
  
  // Button-Labels und Funktionen
  buttons[0].label = "Taster 1";
  buttons[0].instanceID = "17";
  buttons[0].color = BUTTON_COLOR_INACTIVE;
  buttons[0].textColor = TFT_WHITE;
  buttons[0].pressed = false;
  buttons[0].isActive = false;
  
  buttons[1].label = "Taster 2";
  buttons[1].instanceID = "18";
  buttons[1].color = BUTTON_COLOR_INACTIVE;
  buttons[1].textColor = TFT_WHITE;
  buttons[1].pressed = false;
  buttons[1].isActive = false;
  
  buttons[2].label = "Taster 3";
  buttons[2].instanceID = "19";
  buttons[2].color = BUTTON_COLOR_INACTIVE;
  buttons[2].textColor = TFT_WHITE;
  buttons[2].pressed = false;
  buttons[2].isActive = false;
  
  buttons[3].label = "Taster 4";
  buttons[3].instanceID = "20";
  buttons[3].color = BUTTON_COLOR_INACTIVE;
  buttons[3].textColor = TFT_WHITE;
  buttons[3].pressed = false;
  buttons[3].isActive = false;
  
  buttons[4].label = "Taster 5";
  buttons[4].instanceID = "21";
  buttons[4].color = BUTTON_COLOR_INACTIVE;
  buttons[4].textColor = TFT_WHITE;
  buttons[4].pressed = false;
  buttons[4].isActive = false;
  
  buttons[5].label = "Taster 6";
  buttons[5].instanceID = "22";
  buttons[5].color = BUTTON_COLOR_INACTIVE;
  buttons[5].textColor = TFT_WHITE;
  buttons[5].pressed = false;
  buttons[5].isActive = false;
}

// Zeigt das Hauptmenü an
void showMenu() {
  tft.fillScreen(TFT_WHITE);
  
  // *** Header zeichnen ***
  drawHeader();
  
  // *** ENTFERNT: Hauptmenü-Titel ***
  // *** ENTFERNT: Helligkeit-Anzeige ***
  
  // Initialisiere Buttons (mit Header-Offset, aber ohne Titel)
  initButtons();
  
  // Zeichne Buttons
  drawButtons();
  
  // *** ENTFERNT: Test-Button (wird ins Service-Menü integriert) ***
}