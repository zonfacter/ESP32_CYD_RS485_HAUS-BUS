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
// Initialisiert die Buttons mit ihren Positionen und Farben
void initButtons() {
  // *** DYNAMISCHE Button-Berechnung basierend auf aktueller TFT-Größe ***
  int currentWidth = tft.width();
  int currentHeight = tft.height();
  int rotation = tft.getRotation();
  
  #if DB_INFO == 1
    Serial.print("DEBUG: initButtons() - TFT-Rotation: ");
    Serial.print(rotation);
    Serial.print(", Größe: ");
    Serial.print(currentWidth);
    Serial.print("x");
    Serial.println(currentHeight);
  #endif
  
  int headerOffset = HEADER_HEIGHT + 5;
  int availableHeight = currentHeight - headerOffset;

  // *** KORRIGIERTE Orientierungs-Erkennung ***
  if (rotation == 0 || rotation == 2) {
    // Portrait-Rotationen: 2x3 Layout
    buttonWidth = currentWidth / 2;
    buttonHeight = availableHeight / 3;

    #if DB_INFO == 1
      Serial.println("DEBUG: Portrait-Layout (2x3) - Buttons: " + String(buttonWidth) + "x" + String(buttonHeight));
    #endif

    for (int i = 0; i < NUM_BUTTONS; i++) {
      int col = i % 2;
      int row = i / 2;
      int x = col * buttonWidth;
      int y = headerOffset + row * buttonHeight;
      
      // *** KORRIGIERT: Spiegelung nur bei Rotation 2 (Portrait 180°) ***
      if (rotation == 2) {
        x = currentWidth - x - buttonWidth;
        y = currentHeight - y - buttonHeight;
      }
      
      buttons[i].x = x;
      buttons[i].y = y;
      buttons[i].w = buttonWidth;
      buttons[i].h = buttonHeight;
      
      #if DB_INFO == 1
        Serial.printf("DEBUG: Button %d (Portrait): x=%d, y=%d, w=%d, h=%d\n", 
                     i+1, buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h);
      #endif
    }
  } else {
    // Landscape-Rotationen: 3x2 Layout
    buttonWidth = currentWidth / 3;
    buttonHeight = availableHeight / 2;

    #if DB_INFO == 1
      Serial.println("DEBUG: Landscape-Layout (3x2) - Buttons: " + String(buttonWidth) + "x" + String(buttonHeight));
    #endif

    for (int i = 0; i < NUM_BUTTONS; i++) {
      int col = i % 3;
      int row = i / 3;
      int x = col * buttonWidth;
      int y = headerOffset + row * buttonHeight;
      
      // *** KORRIGIERT: Spiegelung nur bei Rotation 3 (Landscape 270° - USB rechts) ***
      if (rotation == 3) {
        x = currentWidth - x - buttonWidth;
      }
      
      buttons[i].x = x;
      buttons[i].y = y;
      buttons[i].w = buttonWidth;
      buttons[i].h = buttonHeight;
      
      #if DB_INFO == 1
        Serial.printf("DEBUG: Button %d (Landscape): x=%d, y=%d, w=%d, h=%d\n", 
                     i+1, buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h);
      #endif
    }
  }

  // *** Button-Labels und IDs setzen (unverändert) ***
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].label = "Taster " + String(i + 1);
    buttons[i].instanceID = String(17 + i);  // IDs 17-22
    buttons[i].color = TFT_DARKGREY;         // Standard: Grau (inaktiv)
    buttons[i].textColor = TFT_WHITE;        // Weiße Schrift
    buttons[i].pressed = false;
    buttons[i].isActive = false;             // Standard: inaktiv
  }

  #if DB_INFO == 1
    Serial.println("=== Button-Layout nach initButtons() ===");
    Serial.printf("Screen: %dx%d, Rotation: %d\n", currentWidth, currentHeight, rotation);
    Serial.printf("Button-Größe: %dx%d\n", buttonWidth, buttonHeight);
    Serial.printf("Header-Offset: %d, Verfügbare Höhe: %d\n", headerOffset, availableHeight);
    Serial.println("=========================================");
  #endif
}


// Zeigt das Hauptmenü an
void showMenu() {
  tft.fillScreen(TFT_WHITE);
  
  // *** Header zeichnen ***
  drawHeader();
  
  // *** Initialisiere Buttons für aktuelle Orientierung ***
  initButtons();
  
  // Zeichne Buttons
  drawButtons();
  
  #if DB_INFO == 1
    Serial.print("DEBUG: showMenu() abgeschlossen - TFT-Rotation: ");
    Serial.print(tft.getRotation());
    Serial.print(", Größe: ");
    Serial.print(tft.width());
    Serial.print("x");
    Serial.println(tft.height());
  #endif
}