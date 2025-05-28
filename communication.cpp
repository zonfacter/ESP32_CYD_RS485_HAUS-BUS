/**
 * communication.cpp - CSMA/CD Implementierung für RS485
 * 
 * Features:
 * - Carrier Sense Multiple Access (CSMA) - Lauschen vor dem Senden
 * - Collision Detection (CD) - Erkennung von Kollisionen
 * - Sendepuffer mit automatischer Wiederholung
 * - Backoff-Algorithmus bei Kollisionen
 * - Priorisierung von Nachrichten
 * - *** NEU: Button-Touch-Priorität für LED-Steuerung ***
 */
#include "communication.h"
#include "backlight.h"
#include "menu.h"
#include "led.h"
#include "service_manager.h"  // NEU: Include für ServiceManager
#include "header_display.h"  // Für Zeit/Datum Funktionen

// Separate UART2-Instanz für RS485
HardwareSerial RS485Serial(2);

// Buffer-Verwaltung für Empfang
char telegramBuffer[MAX_TELEGRAM_LENGTH];
int bufferPos = 0;
bool receivingTelegram = false;

// Timing-Variablen
unsigned long lastByteTime = 0;
unsigned long telegramStartTime = 0;

// CSMA/CD Variablen
bool busIdle = true;
unsigned long lastBusActivity = 0;
const unsigned long BUS_IDLE_TIME = 10;      // 10ms ohne Aktivität = Bus frei
const unsigned long COLLISION_DETECT_TIME = 5; // 5ms nach Sendebeginn auf Kollision prüfen

// Sendepuffer-Struktur
struct SendQueueItem {
  String telegram;
  unsigned long timestamp;
  int retryCount;
  int priority;        // 0=höchste Priorität, 9=niedrigste
  bool urgent;         // Sofort senden (für Antworten)
};

// Sendepuffer (Ring-Buffer) - verwendet #define aus config.h
SendQueueItem sendQueue[SEND_QUEUE_SIZE];
int sendQueueHead = 0;
int sendQueueTail = 0;
int sendQueueCount = 0;

// Statistiken
unsigned long totalSent = 0;
unsigned long totalCollisions = 0;
unsigned long totalRetries = 0;

// *** NEU: Button-Touch-Priorität Variablen ***
// Diese müssen extern deklariert werden, damit sie in main INO zugänglich sind
extern struct ButtonTiming {
  bool touchActive;
  unsigned long touchStartTime;
  bool status1Sent;
  int activeButtonIndex;
} buttonTiming;

// *** NEU: Pending LED State für Buttons ***
struct PendingLedState {
  bool hasPending;
  uint16_t pendingColor;
  bool pendingActive;
};

PendingLedState pendingLedStates[NUM_BUTTONS];

// *** NEU: Hilfsfunktion - Prüft ob Button gerade lokal gedrückt wird ***
bool isButtonLocallyPressed(int buttonIndex) {
  return (buttonTiming.touchActive && 
          buttonTiming.activeButtonIndex == buttonIndex);
}

// *** NEU: Hilfsfunktion - Speichert LED-Status für später ***
void setPendingLedState(int buttonIndex, uint16_t color, bool active) {
  if (buttonIndex >= 0 && buttonIndex < NUM_BUTTONS) {
    pendingLedStates[buttonIndex].hasPending = true;
    pendingLedStates[buttonIndex].pendingColor = color;
    pendingLedStates[buttonIndex].pendingActive = active;
    
    #if DB_RX_INFO == 1
      Serial.print("DEBUG: LED-Status für Button ");
      Serial.print(buttonIndex + 1);
      Serial.println(" gespeichert (lokaler Touch aktiv)");
    #endif
  }
}

// *** NEU: Hilfsfunktion - Wendet gespeicherten LED-Status an ***
void applyPendingLedState(int buttonIndex) {
  if (buttonIndex >= 0 && buttonIndex < NUM_BUTTONS && 
      pendingLedStates[buttonIndex].hasPending) {
    
    buttons[buttonIndex].color = pendingLedStates[buttonIndex].pendingColor;
    buttons[buttonIndex].isActive = pendingLedStates[buttonIndex].pendingActive;
    redrawButton(buttonIndex);
    
    // Pending-Status zurücksetzen
    pendingLedStates[buttonIndex].hasPending = false;
    
    #if DB_RX_INFO == 1
      Serial.print("DEBUG: Gespeicherter LED-Status für Button ");
      Serial.print(buttonIndex + 1);
      Serial.println(" angewendet");
    #endif
  }
}

// *** NEU: Öffentliche Funktion - Pending LED States anwenden (für main INO) ***
void applyAllPendingLedStates() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (pendingLedStates[i].hasPending) {
      applyPendingLedState(i);
    }
  }
}

/**
 * Prüft, ob der Bus frei ist (Carrier Sense)
 */
bool isBusIdle() {
  // Prüfe, ob Daten im Empfangspuffer sind
  if (RS485Serial.available() > 0) {
    lastBusActivity = millis();
    busIdle = false;
    return false;
  }
  
  // Prüfe, ob genug Zeit vergangen ist seit der letzten Aktivität
  if (millis() - lastBusActivity >= BUS_IDLE_TIME) {
    busIdle = true;
    return true;
  }
  
  return false;
}

/**
 * Fügt ein Telegramm zum Sendepuffer hinzu
 */
bool addToSendQueue(const String& telegram, int priority, bool urgent) {
  // Prüfe, ob der Puffer voll ist
  if (sendQueueCount >= SEND_QUEUE_SIZE) {
    #if DB_TX_INFO == 1
      Serial.println("DEBUG: Sendepuffer voll! Telegramm verworfen.");
    #endif
    return false;
  }
  
  // Füge zum Puffer hinzu
  sendQueue[sendQueueHead].telegram = telegram;
  sendQueue[sendQueueHead].timestamp = millis();
  sendQueue[sendQueueHead].retryCount = 0;
  sendQueue[sendQueueHead].priority = priority;
  sendQueue[sendQueueHead].urgent = urgent;
  
  sendQueueHead = (sendQueueHead + 1) % SEND_QUEUE_SIZE;
  sendQueueCount++;
  
  #if DB_TX_INFO == 1
    Serial.print("DEBUG: Telegramm in Sendepuffer, Priorität ");
    Serial.print(priority);
    Serial.print(", Queue-Größe: ");
    Serial.println(sendQueueCount);
  #endif
  
  return true;
}

/**
 * Holt das nächste Telegramm aus dem Sendepuffer (höchste Priorität zuerst)
 */
bool getNextFromSendQueue(SendQueueItem& item) {
  if (sendQueueCount == 0) {
    return false;
  }
  
  // Finde das Element mit der höchsten Priorität (niedrigste Zahl)
  int bestIndex = sendQueueTail;
  int bestPriority = sendQueue[sendQueueTail].priority;
  bool foundUrgent = sendQueue[sendQueueTail].urgent;
  
  // Suche nach dringenden Nachrichten zuerst
  for (int i = 0; i < sendQueueCount; i++) {
    int index = (sendQueueTail + i) % SEND_QUEUE_SIZE;
    
    if (sendQueue[index].urgent && !foundUrgent) {
      bestIndex = index;
      bestPriority = sendQueue[index].priority;
      foundUrgent = true;
    } else if ((sendQueue[index].urgent == foundUrgent) && 
               (sendQueue[index].priority < bestPriority)) {
      bestIndex = index;
      bestPriority = sendQueue[index].priority;
    }
  }
  
  // Kopiere das Element
  item = sendQueue[bestIndex];
  
  // Entferne das Element aus dem Puffer (verschiebe andere nach vorne)
  for (int i = bestIndex; i != sendQueueHead; i = (i + 1) % SEND_QUEUE_SIZE) {
    int nextIndex = (i + 1) % SEND_QUEUE_SIZE;
    if (nextIndex != sendQueueHead) {
      sendQueue[i] = sendQueue[nextIndex];
    }
  }
  
  sendQueueHead = (sendQueueHead - 1 + SEND_QUEUE_SIZE) % SEND_QUEUE_SIZE;
  sendQueueCount--;
  
  return true;
}

/**
 * Berechnet die Backoff-Zeit bei Kollisionen
 */
unsigned long calculateBackoffTime(int retryCount) {
  // Exponential Backoff mit Zufallskomponente
  unsigned long baseTime = 5 + (retryCount * 10);  // 5ms + 10ms pro Retry
  unsigned long randomComponent = random(0, baseTime);
  return baseTime + randomComponent;
}

/**
 * Kollisionserkennung während des Sendens
 */
bool detectCollision(const String& sentTelegram) {
  // Warte kurz, dann prüfe, was tatsächlich gesendet wurde
  delay(COLLISION_DETECT_TIME);
  
  // Prüfe, ob unerwartete Daten empfangen wurden
  if (RS485Serial.available() > 0) {
    String received = "";
    unsigned long startTime = millis();
    
    // Lese alle verfügbaren Daten
    while (RS485Serial.available() > 0 && (millis() - startTime < 20)) {
      received += (char)RS485Serial.read();
      delay(1);
    }
    
    // Vergleiche mit dem gesendeten Telegramm
    if (received != sentTelegram) {
      #if DB_TX_INFO == 1
        Serial.println("DEBUG: Kollision erkannt!");
        Serial.print("Gesendet: ");
        printTelegramHex(sentTelegram);
        Serial.print("Empfangen: ");
        printTelegramHex(received);
      #endif
      
      totalCollisions++;
      return true;
    }
  }
  
  return false;
}

/**
 * Initialisierung mit CSMA/CD-Unterstützung
 */
void setupCommunication() {
  // UART0 für USB-Debug
  Serial.begin(115200);
  delay(100);
  
  // UART2 für RS485
  RS485Serial.begin(57600, SERIAL_8E1, UART_RX_PIN, UART_TX_PIN);
  RS485Serial.setTimeout(10);
  
  delay(100);
  
  // Zufallsgenerator initialisieren
  randomSeed(analogRead(A0) + millis());
  
  // *** NEU: Pending LED States initialisieren ***
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pendingLedStates[i].hasPending = false;
    pendingLedStates[i].pendingColor = TFT_DARKGREY;
    pendingLedStates[i].pendingActive = false;
  }
  
  #if DB_INFO == 1
    Serial.println("\n=== CSMA/CD RS485-Kommunikation ===");
    Serial.println("UART0: USB-Debug (115200, 8N1)");
    Serial.println("UART2: RS485 mit CSMA/CD (57600, 8E1)");
    Serial.print("RS485 RX Pin: ");
    Serial.println(UART_RX_PIN);
    Serial.print("RS485 TX Pin: ");
    Serial.println(UART_TX_PIN);
    Serial.print("Bus Idle Time: ");
    Serial.print(BUS_IDLE_TIME);
    Serial.println(" ms");
    Serial.print("Sendepuffer-Größe: ");
    Serial.println(SEND_QUEUE_SIZE);
    Serial.println("NEU: Button-Touch-Priorität für LED-Steuerung");
    Serial.println("CSMA/CD initialisiert");
  #endif
  
  // Buffer zurücksetzen
  bufferPos = 0;
  receivingTelegram = false;
  lastBusActivity = millis();
  
  // Sendepuffer initialisieren
  sendQueueHead = 0;
  sendQueueTail = 0;
  sendQueueCount = 0;
  
  // RS485-Empfangspuffer leeren
  while (RS485Serial.available()) {
    RS485Serial.read();
  }
  
  delay(100);
}

/**
 * Hauptfunktion für das Senden mit CSMA/CD
 */
bool transmitWithCSMA(const String& telegram, int maxRetries) {
  for (int attempt = 0; attempt < maxRetries; attempt++) {
    // 1. Carrier Sense - Warte, bis der Bus frei ist
    unsigned long waitStart = millis();
    while (!isBusIdle()) {
      delay(1);
      // Timeout nach 100ms
      if (millis() - waitStart > 100) {
        #if DB_TX_INFO == 1
          Serial.println("DEBUG: Timeout beim Warten auf freien Bus");
        #endif
        return false;
      }
    }
    
    // 2. Zusätzliche zufällige Wartezeit (um Kollisionen zu vermeiden)
    if (attempt > 0) {
      unsigned long backoffTime = calculateBackoffTime(attempt);
      #if DB_TX_INFO == 1
        Serial.print("DEBUG: Backoff-Zeit: ");
        Serial.print(backoffTime);
        Serial.println(" ms");
      #endif
      delay(backoffTime);
      
      // Erneut prüfen, ob der Bus noch frei ist
      if (!isBusIdle()) {
        continue;  // Nächster Versuch
      }
    }
    
    // 3. Senden
    #if DB_TX_HEX == 1
      Serial.print("DEBUG: Sende Telegramm (Versuch ");
      Serial.print(attempt + 1);
      Serial.print("): ");
      printTelegramHex(telegram);
    #endif
    
    RS485Serial.print(telegram);
    RS485Serial.flush();
    
    // LED-Signal
    ledSendSignal();
    
    // 4. Collision Detection
    if (!detectCollision(telegram)) {
      // Erfolgreich gesendet
      totalSent++;
      
      #if DB_TX_INFO == 1
        Serial.println("DEBUG: Telegramm erfolgreich gesendet");
      #endif
      
      return true;
    } else {
      // Kollision erkannt
      totalRetries++;
      
      #if DB_TX_INFO == 1
        Serial.print("DEBUG: Kollision bei Versuch ");
        Serial.print(attempt + 1);
        Serial.println(", wiederhole...");
      #endif
    }
  }
  
  // Alle Versuche fehlgeschlagen
  #if DB_TX_INFO == 1
    Serial.println("DEBUG: Senden fehlgeschlagen nach allen Versuchen");
  #endif
  
  return false;
}

/**
 * Öffentliche Sendefunktion - fügt zum Puffer hinzu
 */
void sendTelegram(String function, String instanceID, String action, String params) {
  // *** KORRIGIERT: Device ID vom ServiceManager holen ***
  String currentDeviceID = serviceManager.getDeviceID();
  
  // Telegramm aufbauen mit aktueller Device ID
  String telegram = String((char)START_BYTE) + currentDeviceID + "." + function + "." + instanceID + "." + action;
  
  if (params != "") {
    telegram += "." + params;
  }
  
  telegram += String((char)END_BYTE);
  
  #if DB_TX_INFO == 1
    Serial.print("DEBUG: Sende Telegramm mit Device ID ");
    Serial.print(currentDeviceID);
    Serial.print(": ");
    Serial.println(telegram);
  #endif
  
  // Priorität bestimmen
  int priority = 5;  // Standard-Priorität
  bool urgent = false;
  
  if (function == "BTN") {
    priority = 1;  // Taster haben hohe Priorität
  } else if (function == "BLT" && action == "STATUS") {
    priority = 7;  // Status-Nachrichten haben niedrige Priorität
    urgent = false;
  }
  
  // Zum Sendepuffer hinzufügen
  if (!addToSendQueue(telegram, priority, urgent)) {
    #if DB_TX_INFO == 1
      Serial.println("DEBUG: Konnte Telegramm nicht zum Sendepuffer hinzufügen");
    #endif
  }
}

/**
 * Sendepuffer abarbeiten - muss regelmäßig aufgerufen werden
 */
void processSendQueue() {
  static unsigned long lastProcessTime = 0;
  
  // Nur alle 2ms prüfen, um CPU zu schonen
  if (millis() - lastProcessTime < 2) {
    return;
  }
  lastProcessTime = millis();
  
  // Prüfe, ob etwas zu senden ist
  if (sendQueueCount == 0) {
    return;
  }
  
  // Prüfe, ob der Bus frei ist
  if (!isBusIdle()) {
    return;
  }
  
  // Hole das nächste Telegramm
  SendQueueItem item;
  if (getNextFromSendQueue(item)) {
    // Versuche zu senden
    if (!transmitWithCSMA(item.telegram, 3)) {
      // Senden fehlgeschlagen - zurück in den Puffer wenn noch Versuche übrig
      item.retryCount++;
      
      if (item.retryCount < 5) {  // Maximal 5 Versuche
        // Mit niedrigerer Priorität zurück in den Puffer
        item.priority = min(item.priority + 1, 9);
        
        if (!addToSendQueue(item.telegram, item.priority, false)) {
          #if DB_TX_INFO == 1
            Serial.println("DEBUG: Konnte fehlgeschlagenes Telegramm nicht erneut einreihen");
          #endif
        }
      } else {
        #if DB_TX_INFO == 1
          Serial.println("DEBUG: Telegramm nach 5 Versuchen verworfen");
        #endif
      }
    }
  }
}

/**
 * Erweiterte Empfangsfunktion
 */
void processIncomingTelegrams() {
  // Timeout für unvollständige Telegramme
  if (receivingTelegram && (millis() - telegramStartTime > TELEGRAM_TIMEOUT_MS)) {
    receivingTelegram = false;
    bufferPos = 0;
    #if DB_RX_INFO == 1
      Serial.println("DEBUG: Telegramm-Timeout, Empfang abgebrochen");
    #endif
  }
  
  // Überprüfen, ob Daten auf RS485 verfügbar sind
  if (!RS485Serial.available()) {
    return;
  }
  
  // Bus-Aktivität markieren
  lastBusActivity = millis();
  busIdle = false;
  
  #if DB_RX_INFO == 1
    Serial.print("DEBUG: RS485 Daten verfügbar: ");
    Serial.print(RS485Serial.available());
    Serial.println(" Bytes");
  #endif
  
  // Lese alle verfügbaren Bytes von RS485
  while (RS485Serial.available() > 0) {
    uint8_t byteValue = RS485Serial.read();
    char c = (char)byteValue;
    
    // Nullbyte-Filterung
    if (byteValue == 0) {
      #if DB_RX_INFO == 1
        Serial.println("DEBUG: Nullbyte gefiltert");
      #endif
      continue;
    }
    
    // Debug-Ausgabe für jedes empfangene Byte
    #if DB_RX_HEX == 1
      Serial.print("DEBUG: Byte: 0x");
      if (byteValue < 16) Serial.print("0");
      Serial.print(byteValue, HEX);
      Serial.print(" (");
      if (c >= 32 && c <= 126) {
        Serial.print(c);
      } else {
        Serial.print(".");
      }
      Serial.println(")");
    #endif
    
    // Verarbeitung des Bytes
    if (byteValue == START_BYTE) {
      // Start eines neuen Telegramms
      receivingTelegram = true;
      bufferPos = 0;
      telegramBuffer[bufferPos++] = c;
      telegramStartTime = millis();
      
      #if DB_RX_INFO == 1
        Serial.println("DEBUG: Neues Telegramm gestartet");
      #endif
    }
    else if (receivingTelegram) {
      // Puffer-Überlauf verhindern
      if (bufferPos < MAX_TELEGRAM_LENGTH - 1) {
        telegramBuffer[bufferPos++] = c;
        
        // Ende des Telegramms erkannt
        if (byteValue == END_BYTE) {
          telegramBuffer[bufferPos] = '\0';
          
          String telegramStr = String(telegramBuffer);
          
          #if DB_RX_INFO == 1
            Serial.print("DEBUG: Telegramm vollständig empfangen: ");
            Serial.println(telegramStr);
            printTelegramHex(telegramStr);
          #endif
          
          // Telegramm verarbeiten
          processTelegram(telegramStr);
          
          // Zurücksetzen für das nächste Telegramm
          receivingTelegram = false;
          bufferPos = 0;
        }
      }
      else {
        // Puffer-Überlauf - Empfang abbrechen
        receivingTelegram = false;
        bufferPos = 0;
        #if DB_RX_INFO == 1
          Serial.println("DEBUG: Telegramm zu lang, verworfen");
        #endif
      }
    }
    // Zeichen außerhalb eines Telegramms werden ignoriert
  }
}

/**
 * Statistiken ausgeben
 */
void printCommunicationStats() {
  #if DB_INFO == 1
    Serial.println("\n=== Kommunikations-Statistiken ===");
    Serial.print("Gesendete Telegramme: ");
    Serial.println(totalSent);
    Serial.print("Erkannte Kollisionen: ");
    Serial.println(totalCollisions);
    Serial.print("Wiederholungen: ");
    Serial.println(totalRetries);
    Serial.print("Sendepuffer-Status: ");
    Serial.print(sendQueueCount);
    Serial.print("/");
    Serial.println(SEND_QUEUE_SIZE);
    Serial.print("Bus-Status: ");
    Serial.println(busIdle ? "Frei" : "Belegt");
    Serial.println("================================");
  #endif
}

/**
 * Hauptupdate-Funktion - muss regelmäßig aufgerufen werden
 */
void updateCommunication() {
  // Sendepuffer abarbeiten
  processSendQueue();
  
  // Empfangene Telegramme verarbeiten
  processIncomingTelegrams();
  
  // Statistiken alle 30 Sekunden ausgeben
  static unsigned long lastStatsTime = 0;
  if (millis() - lastStatsTime > 30000) {
    printCommunicationStats();
    lastStatsTime = millis();
  }
}

/**
 * *** KORRIGIERTE processTelegram() Funktion mit Button-Touch-Priorität ***
 */
void processTelegram(String telegramStr) {
  // Überprüfen, ob das Telegramm das richtige Format hat
  if (telegramStr.length() < 10) {
    #if DB_RX_INFO == 1
      Serial.print("DEBUG: Telegramm zu kurz: ");
      Serial.println(telegramStr);
    #endif
    return;
  }

  if (telegramStr.charAt(0) != START_BYTE || 
      telegramStr.charAt(telegramStr.length() - 1) != END_BYTE) {
    #if DB_RX_INFO == 1
      Serial.println("DEBUG: Telegramm hat ungültiges Format (START/END)");
    #endif
    return;
  }

  // LED-Signal für den Empfang aktivieren
  ledReceiveSignal();

  // Entferne START_BYTE und END_BYTE für die weitere Verarbeitung
  String payload = telegramStr.substring(1, telegramStr.length() - 1);

  #if DB_RX_INFO == 1
    Serial.print("DEBUG: Payload: ");
    Serial.println(payload);
  #endif

  // Payload in Teile zerlegen (Format: DEVICE_ID.FUNCTION.INSTANCE_ID.ACTION.PARAMS)
  int firstDot = payload.indexOf('.');
  if (firstDot == -1) return;

  String deviceId = payload.substring(0, firstDot);
  String remainder = payload.substring(firstDot + 1);

  // Aktuelle Device ID vom ServiceManager holen
  String currentDeviceID = serviceManager.getDeviceID();
  
  // Prüfen, ob es unser Gerät ist
  if (deviceId != currentDeviceID) {
    #if DB_RX_INFO == 1
      Serial.print("DEBUG: Telegramm nicht für uns - empfangen für Device ID: ");
      Serial.print(deviceId);
      Serial.print(", unsere ID: ");
      Serial.println(currentDeviceID);
    #endif
    return;
  }

  // *** NEU: Service-Mode Check - nur bestimmte Funktionen erlauben ***
  bool isServiceMode = serviceManager.isServiceMode();
  
  // Weitere Zerlegung für Funktions-Check
  int secondDot = remainder.indexOf('.');
  if (secondDot == -1) return;
  String function = remainder.substring(0, secondDot);

  if (isServiceMode) {
    // *** IM SERVICE-MODUS: Nur System-Funktionen erlauben ***
    if (function != "SYS" && function != "TIME" && function != "DATE") {
      #if DB_RX_INFO == 1
        Serial.print("DEBUG: Service-Modus aktiv - ");
        Serial.print(function);
        Serial.println("-Telegramm wird blockiert (nur SYS/TIME/DATE erlaubt)");
      #endif
      return;  // LED, LBN, BTN Telegramme werden im Service-Modus ignoriert
    }
    
    #if DB_RX_INFO == 1
      Serial.print("DEBUG: Service-Modus aktiv - ");
      Serial.print(function);
      Serial.println("-Telegramm wird verarbeitet");
    #endif
  }

  #if DB_RX_INFO == 1
    Serial.print("DEBUG: Telegramm für uns! Device ID: ");
    Serial.print(currentDeviceID);
    Serial.print(" - Verarbeite: ");
    Serial.println(remainder);
  #endif

  // Rest der Zerlegung
  String rest = remainder.substring(secondDot + 1);

  int thirdDot = rest.indexOf('.');
  if (thirdDot == -1) return;

  String instanceId = rest.substring(0, thirdDot);
  String actionAndParams = rest.substring(thirdDot + 1);

  int fourthDot = actionAndParams.indexOf('.');
  String action = (fourthDot == -1) ? actionAndParams : actionAndParams.substring(0, fourthDot);
  String params = (fourthDot == -1) ? "" : actionAndParams.substring(fourthDot + 1);

  #if DB_RX_INFO == 1
    Serial.print("DEBUG: Function: ");
    Serial.println(function);
    Serial.print("DEBUG: InstanceID: ");
    Serial.println(instanceId);
    Serial.print("DEBUG: Action: ");
    Serial.println(action);
    Serial.print("DEBUG: Params: ");
    Serial.println(params);
  #endif

  // Funktionen verarbeiten
  if (function == "LBN") {
    // Backlight-Steuerung (nur im Normal-Modus)
    if (action == "SET_MBR") {
      int brightness = params.toInt();
      if (brightness >= 0 && brightness <= 100) {
        setBacklight(brightness);
        #if DB_RX_INFO == 1
          Serial.print("DEBUG: Backlight auf ");
          Serial.print(brightness);
          Serial.println("% gesetzt");
        #endif
      }
    } else if (action == "GET") {
      // Status zurücksenden
      sendBacklightStatus();
    }
  }
  else if (function == "SYS") {
    // System-Steuerung (immer erlaubt)
    if (action == "RESET") {
      #if DB_RX_INFO == 1
        Serial.println("DEBUG: SYSTEM RESET empfangen!");
        Serial.println("DEBUG: ESP32 wird in 2 Sekunden neu gestartet...");
        Serial.flush();
      #endif
      
      delay(2000);
      ESP.restart();
    } else if (action == "SERVICE" || action == "WIFI" || action == "WEBSERVER" || 
               action == "DEVICE_ID" || action == "ORIENTATION") {
      #if DB_RX_INFO == 1
        Serial.print("DEBUG: SYS.");
        Serial.print(action);
        Serial.print(" Telegramm empfangen - Params: ");
        Serial.println(params);
      #endif
      
      serviceManager.handleServiceTelegram(action, params);
    }
  }
  else if (function == "LED") {
    // *** KORRIGIERTE LED-Steuerung mit Button-Touch-Priorität ***
    int ledId = instanceId.toInt();
    if (ledId >= 49 && ledId <= 54) {  // LED-IDs 49-54
      int buttonIndex = ledId - 49;    // Button-Index 0-5 (Button 1-6)
      
      if (buttonIndex >= 0 && buttonIndex < NUM_BUTTONS) {
        
        // *** NEUE LOGIK: Prüfe ob Button gerade lokal gedrückt wird ***
        if (isButtonLocallyPressed(buttonIndex)) {
          #if DB_RX_INFO == 1
            Serial.print("DEBUG: LED-Telegramm für Button ");
            Serial.print(buttonIndex + 1);
            Serial.println(" empfangen, aber Button ist lokal aktiv - speichere für später");
          #endif
          
          // Button ist gerade lokal aktiv - speichere LED-Status für später
          if (action == "ON") {
            int brightness = params.toInt();
            brightness = constrain(brightness, 0, 100);
            
            if (brightness > 0) {
              // Helligkeit > 0 → Weiß mit entsprechender Helligkeit
              uint8_t level = map(brightness, 0, 100, 0, 255);
              uint16_t whiteColor = tft.color565(level, level, level);
              setPendingLedState(buttonIndex, whiteColor, true);
            } else {
              // Helligkeit = 0 → Grau (deaktiviert)
              setPendingLedState(buttonIndex, TFT_DARKGREY, false);
            }
          } else if (action == "OFF") {
            setPendingLedState(buttonIndex, TFT_DARKGREY, false);
          }
          
          // NICHT sofort anwenden - wird nach Button-Release angewendet
          return;
        }
        
        // *** NORMALE LED-Verarbeitung (Button nicht lokal aktiv) ***
        if (action == "ON") {
          int brightness = params.toInt();
          brightness = constrain(brightness, 0, 100);
          
          if (brightness > 0) {
            // Helligkeit > 0 → Weiß mit entsprechender Helligkeit
            uint8_t level = map(brightness, 0, 100, 0, 255);
            uint16_t whiteColor = tft.color565(level, level, level);
            buttons[buttonIndex].color = whiteColor;
            buttons[buttonIndex].isActive = true;  // Button als aktiv markieren
            #if DB_RX_INFO == 1
              Serial.print("DEBUG: LED ");
              Serial.print(ledId);
              Serial.print(" (Button ");
              Serial.print(buttonIndex + 1);
              Serial.print(") aktiviert mit Device ID ");
              Serial.print(currentDeviceID);
              Serial.print(" - Weiß mit Helligkeit ");
              Serial.print(brightness);
              Serial.print("% (RGB: ");
              Serial.print(level);
              Serial.println(")");
            #endif
          } else {
            // Helligkeit = 0 → Grau (deaktiviert)
            buttons[buttonIndex].color = TFT_DARKGREY;
            buttons[buttonIndex].isActive = false;  // Button als inaktiv markieren
            #if DB_RX_INFO == 1
              Serial.print("DEBUG: LED ");
              Serial.print(ledId);
              Serial.print(" (Button ");
              Serial.print(buttonIndex + 1);
              Serial.print(") deaktiviert mit Device ID ");
              Serial.print(currentDeviceID);
              Serial.println(" - ON.0 = Grau");
            #endif
          }
          redrawButton(buttonIndex);
          
        } else if (action == "OFF") {
          buttons[buttonIndex].color = TFT_DARKGREY;
          buttons[buttonIndex].isActive = false;  // Button als inaktiv markieren
          redrawButton(buttonIndex);
          #if DB_RX_INFO == 1
            Serial.print("DEBUG: LED ");
            Serial.print(ledId);
            Serial.print(" (Button ");
            Serial.print(buttonIndex + 1);
            Serial.print(") deaktiviert mit Device ID ");
            Serial.print(currentDeviceID);
            Serial.println(" - OFF = Grau");
          #endif
        }
      }
    } else {
      #if DB_RX_INFO == 1
        Serial.print("DEBUG: Ungültige LED-ID: ");
        Serial.print(ledId);
        Serial.println(" (erwartet: 49-54)");
      #endif
    }
  }
  else if (function == "TIME") {
    // Zeit-Steuerung (immer erlaubt)
    if (action == "SET") {
      handleTimeSetTelegram(params);
    } else if (action == "GET") {
      String timeStr = String(currentTime.hour < 10 ? "0" : "") + String(currentTime.hour) +
                      String(currentTime.minute < 10 ? "0" : "") + String(currentTime.minute) +
                      String(currentTime.second < 10 ? "0" : "") + String(currentTime.second);
      
      sendTelegram("TIME", "STATUS", timeStr, "");
      
      #if DB_RX_INFO == 1
        Serial.print("DEBUG: Zeit-Status gesendet: ");
        Serial.println(timeStr);
      #endif
    }
  }
  else if (function == "DATE") {
    // Datum-Steuerung (immer erlaubt)
    if (action == "SET") {
      handleDateSetTelegram(params);
    } else if (action == "GET") {
      String dateStr = String(currentTime.day < 10 ? "0" : "") + String(currentTime.day) +
                      String(currentTime.month < 10 ? "0" : "") + String(currentTime.month) +
                      String(currentTime.year);
      
      sendTelegram("DATE", "STATUS", dateStr, "");
      
      #if DB_RX_INFO == 1
        Serial.print("DEBUG: Datum-Status gesendet: ");
        Serial.println(dateStr);
      #endif
    }
  }
  else if (function == "BTN") {
    // Button-Status (nur im Normal-Modus, bereits durch Service-Check blockiert)
    #if DB_RX_INFO == 1
      Serial.println("DEBUG: Button-Status empfangen (ungewöhnlich)");
    #endif
  }

  #if DB_RX_INFO == 1
    Serial.println("DEBUG: Telegramm-Verarbeitung abgeschlossen");
  #endif
}

/**
 * Hex-Ausgabe für Debugging
 */
void printTelegramHex(String telegram) {
  Serial.print("HEX: ");
  for (int i = 0; i < telegram.length(); i++) {
    uint8_t byteValue = (uint8_t)telegram.charAt(i);
    if (byteValue < 16) Serial.print("0");
    Serial.print(byteValue, HEX);
    Serial.print(" ");
  }
  Serial.println();
}