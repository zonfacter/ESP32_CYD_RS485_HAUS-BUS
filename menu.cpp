#include "menu.h"
#include "backlight.h"

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
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) return;
  
  buttons[buttonIndex].isActive = active;
  redrawButton(buttonIndex);
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
  #if SCREEN_ORIENTATION == PORTRAIT
    // Portrait: 2 Spalten x 3 Zeilen
    buttonWidth = SCREEN_WIDTH / 2;
    buttonHeight = SCREEN_HEIGHT / 3;
    
    // Button 1 (oben links)
    buttons[0].x = 0;
    buttons[0].y = 0;
    buttons[0].w = buttonWidth;
    buttons[0].h = buttonHeight;
    
    // Button 2 (oben rechts)
    buttons[1].x = buttonWidth;
    buttons[1].y = 0;
    buttons[1].w = buttonWidth;
    buttons[1].h = buttonHeight;
    
    // Button 3 (mitte links)
    buttons[2].x = 0;
    buttons[2].y = buttonHeight;
    buttons[2].w = buttonWidth;
    buttons[2].h = buttonHeight;
    
    // Button 4 (mitte rechts)
    buttons[3].x = buttonWidth;
    buttons[3].y = buttonHeight;
    buttons[3].w = buttonWidth;
    buttons[3].h = buttonHeight;
    
    // Button 5 (unten links)
    buttons[4].x = 0;
    buttons[4].y = buttonHeight * 2;
    buttons[4].w = buttonWidth;
    buttons[4].h = buttonHeight;
    
    // Button 6 (unten rechts)
    buttons[5].x = buttonWidth;
    buttons[5].y = buttonHeight * 2;
    buttons[5].w = buttonWidth;
    buttons[5].h = buttonHeight;
  #else
    // Landscape: 3 Spalten x 2 Zeilen
    buttonWidth = SCREEN_WIDTH / 3;
    buttonHeight = SCREEN_HEIGHT / 2;
    
    // Button 1 (oben links)
    buttons[0].x = 0;
    buttons[0].y = 0;
    buttons[0].w = buttonWidth;
    buttons[0].h = buttonHeight;
    
    // Button 2 (oben mitte)
    buttons[1].x = buttonWidth;
    buttons[1].y = 0;
    buttons[1].w = buttonWidth;
    buttons[1].h = buttonHeight;
    
    // Button 3 (oben rechts)
    buttons[2].x = buttonWidth * 2;
    buttons[2].y = 0;
    buttons[2].w = buttonWidth;
    buttons[2].h = buttonHeight;
    
    // Button 4 (unten links)
    buttons[3].x = 0;
    buttons[3].y = buttonHeight;
    buttons[3].w = buttonWidth;
    buttons[3].h = buttonHeight;
    
    // Button 5 (unten mitte)
    buttons[4].x = buttonWidth;
    buttons[4].y = buttonHeight;
    buttons[4].w = buttonWidth;
    buttons[4].h = buttonHeight;
    
    // Button 6 (unten rechts)
    buttons[5].x = buttonWidth * 2;
    buttons[5].y = buttonHeight;
    buttons[5].w = buttonWidth;
    buttons[5].h = buttonHeight;
  #endif
  
  // Button-Labels und Funktionen
  buttons[0].label = "Taster 1";
  buttons[0].instanceID = "1";
  buttons[0].color = BUTTON_COLOR_INACTIVE;  // Standardfarbe ist grau
  buttons[0].textColor = TFT_WHITE;
  buttons[0].pressed = false;
  buttons[0].isActive = false;
  
  buttons[1].label = "Taster 2";
  buttons[1].instanceID = "2";
  buttons[1].color = BUTTON_COLOR_INACTIVE;  // Standardfarbe ist grau
  buttons[1].textColor = TFT_WHITE;
  buttons[1].pressed = false;
  buttons[1].isActive = false;
  
  buttons[2].label = "Taster 3";
  buttons[2].instanceID = "3";
  buttons[2].color = BUTTON_COLOR_INACTIVE;  // Standardfarbe ist grau
  buttons[2].textColor = TFT_WHITE;
  buttons[2].pressed = false;
  buttons[2].isActive = false;
  
  buttons[3].label = "Taster 4";
  buttons[3].instanceID = "4";
  buttons[3].color = BUTTON_COLOR_INACTIVE;  // Standardfarbe ist grau
  buttons[3].textColor = TFT_WHITE;
  buttons[3].pressed = false;
  buttons[3].isActive = false;
  
  buttons[4].label = "Taster 5";
  buttons[4].instanceID = "5";
  buttons[4].color = BUTTON_COLOR_INACTIVE;  // Standardfarbe ist grau
  buttons[4].textColor = TFT_WHITE;
  buttons[4].pressed = false;
  buttons[4].isActive = false;
  
  buttons[5].label = "Taster 6";
  buttons[5].instanceID = "6";
  buttons[5].color = BUTTON_COLOR_INACTIVE;  // Standardfarbe ist grau
  buttons[5].textColor = TFT_WHITE;
  buttons[5].pressed = false;
  buttons[5].isActive = false;
}

// Zeigt das Hauptmenü an
void showMenu() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Hauptmenu", SCREEN_WIDTH/2, 10, 2);
  
  // Initialisiere Buttons
  initButtons();
  
  // Zeichne Buttons
  drawButtons();
  
  // Testbutton hinzufügen
  tft.fillRect(SCREEN_WIDTH - 60, 5, 55, 30, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("TEST", SCREEN_WIDTH - 32, 15, 1);
  
  // Helligkeit anzeigen
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString("Helligkeit: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 20, 1);
}