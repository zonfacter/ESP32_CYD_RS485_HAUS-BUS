#ifndef MENU_H
#define MENU_H

#include "config.h"

// Zeichnet alle Buttons
void drawButtons();

// Zeichnet einen einzelnen Button neu (für Statusaktualisierungen)
void redrawButton(int buttonIndex);

// Setzt den Aktivierungsstatus eines Buttons und zeichnet ihn neu
void setButtonActive(int buttonIndex, bool active);

// Findet einen Button anhand seiner Instanz-ID
int findButtonByInstanceID(String instanceID);

// Prüft, ob ein Button gedrückt wurde
int checkButtonPress(int x, int y);

// Initialisiert die Buttons mit ihren Positionen und Farben
void initButtons();

// Zeigt das Hauptmenü an
void showMenu();

#endif // MENU_H