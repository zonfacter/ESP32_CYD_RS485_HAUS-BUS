#include "led.h"
#include "User_Setup.h"

// Globale Variablen für den LED-Status
unsigned long ledEndTime = 0;
int currentLedState = 0;  // 0=aus, 1=rot (senden), 2=blau (empfangen)

// Initialisiert die RGB-LED
void setupLed() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  
  // Alle LEDs ausschalten (HIGH = aus für negierte Logik)
  digitalWrite(LED_RED_PIN, HIGH);
  digitalWrite(LED_GREEN_PIN, HIGH);
  digitalWrite(LED_BLUE_PIN, HIGH);
  
  Serial.println("RGB-LED initialisiert");
}

// Schaltet alle LED-Farben aus
void ledOff() {
  digitalWrite(LED_RED_PIN, HIGH);    // Aus bei negierter Logik
  digitalWrite(LED_GREEN_PIN, HIGH);  // Aus bei negierter Logik
  digitalWrite(LED_BLUE_PIN, HIGH);   // Aus bei negierter Logik
  currentLedState = 0;
}

// Schaltet die rote LED ein (für das Senden)
void ledSendSignal() {
  // Alle LEDs ausschalten
  digitalWrite(LED_GREEN_PIN, HIGH);  // Aus bei negierter Logik
  digitalWrite(LED_BLUE_PIN, HIGH);   // Aus bei negierter Logik
  
  // Rote LED einschalten
  digitalWrite(LED_RED_PIN, LOW);     // Ein bei negierter Logik
  
  // Timer setzen
  ledEndTime = millis() + LED_SEND_DURATION;
  currentLedState = 1;
  #if DB_TX_INFO == 1
  Serial.println("LED: Rot (Senden)");
  #endif
}

// Schaltet die blaue LED ein (für den Empfang)
void ledReceiveSignal() {
  // Alle LEDs ausschalten
  digitalWrite(LED_RED_PIN, HIGH);    // Aus bei negierter Logik
  digitalWrite(LED_GREEN_PIN, HIGH);  // Aus bei negierter Logik
  
  // Blaue LED einschalten
  digitalWrite(LED_BLUE_PIN, LOW);    // Ein bei negierter Logik
  
  // Timer setzen
  ledEndTime = millis() + LED_RECEIVE_DURATION;
  currentLedState = 2;
  #if DB_RX_INFO == 1
  Serial.println("LED: Blau (Empfangen)");
  #endif
}

// Aktualisiert den LED-Status basierend auf Timern
void updateLedStatus() {
  // Wenn die LED eingeschaltet ist und die Zeit abgelaufen ist
  unsigned long currentTime = millis();
  
  // Schutz vor Division durch Null und Überlauf
  if (ledEndTime == 0) {
    // LED ist aus oder wurde nie eingeschaltet
    return;
  }
  
  unsigned long timeSinceLedStart;
  if (currentTime >= ledEndTime) {
    // Zeit ist abgelaufen
    if (currentLedState != 0) {
      ledOff();
    }
  }
}