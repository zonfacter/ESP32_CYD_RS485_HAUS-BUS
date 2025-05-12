#include "communication.h"
#include "backlight.h"
#include "menu.h"
#include "led.h"

// Hardware Serial für UART-Pins definieren
// UART2 verwenden
HardwareSerial SerialPort(2); // UART2 am ESP32

// Puffer für eingehende Telegramme
char telegramBuffer[MAX_TELEGRAM_LENGTH];
int bufferPos = 0;
bool receivingTelegram = false;

// Hilfsfunktion zur HEX-Ausgabe von Telegrammen
void printTelegramHex(String telegram) {
  for (int i = 0; i < telegram.length(); i++) {
    Serial.print("0x");
    Serial.print((byte)telegram.charAt(i), HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// Verarbeitet Bytes von einer seriellen Schnittstelle
void processSerialBytes(Stream &serialPort) {
  while (serialPort.available() > 0) {
    char c = serialPort.read();
    
    // Start eines neuen Telegramms erkannt
    if (c == START_BYTE) {
      receivingTelegram = true;
      bufferPos = 0;
      telegramBuffer[bufferPos++] = c;
    }
    // Weiteres Zeichen eines Telegramms
    else if (receivingTelegram) {
      // Puffer-Überlauf verhindern
      if (bufferPos < MAX_TELEGRAM_LENGTH - 1) {
        telegramBuffer[bufferPos++] = c;
        
        // Ende des Telegramms erkannt
        if (c == END_BYTE) {
          telegramBuffer[bufferPos] = '\0';  // String-Terminierung
          
          // Telegramm verarbeiten
          String telegramStr = String(telegramBuffer);
          processTelegram(telegramStr);
          
          // Zurücksetzen für das nächste Telegramm
          receivingTelegram = false;
          bufferPos = 0;
        }
      }
      // Puffer-Überlauf - Empfang abbrechen
      else {
        receivingTelegram = false;
        bufferPos = 0;
        Serial.println("Telegramm zu lang, verworfen");
      }
    }
    // Ignoriere Zeichen außerhalb eines Telegramms
  }
}

// Initialisiert die Kommunikation (serielle Schnittstellen)
void setupCommunication() {
  // Standard-Serial (USB) ist bereits in setup() initialisiert
  
  // Zusätzliche Hardware-Serial für UART-Pins initialisieren
  // RX, TX entsprechend der TZT ESP32 Dokumentation
  SerialPort.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  
  // Enable-Pin konfigurieren, falls benötigt
  pinMode(UART_EN_PIN, OUTPUT);
  digitalWrite(UART_EN_PIN, HIGH); // Enable UART (HIGH = aktiv, kann je nach Hardware variieren)
  
  Serial.println("Kommunikation initialisiert: USB und UART2");
  Serial.println("UART-Pins: RX=" + String(UART_RX_PIN) + ", TX=" + String(UART_TX_PIN) + ", EN=" + String(UART_EN_PIN));
  Serial.println("START_BYTE: 0x" + String(START_BYTE, HEX) + ", END_BYTE: 0x" + String(END_BYTE, HEX));
}

// Funktion zum Senden eines Protokoll-Telegramms
void sendTelegram(String function, String instanceID, String action, String params) {
  // Telegramm ohne Leerzeichen zwischen START_BYTE/END_BYTE und Daten
  String telegram = String((char)START_BYTE) + DEVICE_ID + "." + function + "." + instanceID + "." + action;
  
  if (params != "") {
    telegram += "." + params;
  }
  
  telegram += String((char)END_BYTE);
  
  // Sende an beide serielle Schnittstellen
  Serial.print("Sende Telegramm: ");
  printTelegramHex(telegram);
  
  Serial.print(telegram); // Sende an USB ohne Zeilenumbruch
  SerialPort.print(telegram); // Sende an UART ohne Zeilenumbruch
  
  // LED-Signal für das Senden aktivieren
  ledSendSignal();
}

// Hilfsfunktion zur Verarbeitung eines Telegramms
void processTelegram(String telegramStr) {
  // Überprüfen, ob das Telegramm das richtige Format hat und mindestens 10 Zeichen lang ist
  // (START_BYTE + DEVICE_ID + Minimal-Struktur + END_BYTE)
  if (telegramStr.length() < 10) {
    Serial.println("Telegramm zu kurz: " + telegramStr);
    return;
  }
  
  if (telegramStr.charAt(0) != START_BYTE || telegramStr.charAt(telegramStr.length() - 1) != END_BYTE) {
    Serial.println("Telegramm hat ungültiges Format (START/END)");
    return;
  }
  
  // Überprüfen, ob die Geräte-ID übereinstimmt
  if (telegramStr.indexOf(DEVICE_ID) != 1) {  // Position direkt nach START_BYTE
    Serial.println("Falsche Geräte-ID im Telegramm");
    return;
  }
  
  // Debug-Ausgabe des empfangenen Telegramms
  Serial.print("Empfangenes Telegramm: ");
  printTelegramHex(telegramStr);
  
  // LED-Signal für den Empfang aktivieren
  ledReceiveSignal();
  
  // Entferne START_BYTE und END_BYTE für die weitere Verarbeitung
  String payload = telegramStr.substring(1, telegramStr.length() - 1);
  Serial.println("Payload: " + payload);
  
  // Zerlege das Telegramm in seine Bestandteile
  int firstDot = payload.indexOf('.');
  int secondDot = payload.indexOf('.', firstDot + 1);
  int thirdDot = payload.indexOf('.', secondDot + 1);
  
  if (firstDot <= 0 || secondDot <= 0 || thirdDot <= 0) {
    Serial.println("Telegramm hat ungültiges Format (Struktur)");
    return;
  }
  
  String deviceID = payload.substring(0, firstDot);
  String function = payload.substring(firstDot + 1, secondDot);
  String instanceID = payload.substring(secondDot + 1, thirdDot);
  
  // Prüfe, ob es einen Parameter gibt
  int fourthDot = payload.indexOf('.', thirdDot + 1);
  String action;
  String params = "";
  
  if (fourthDot > 0) {
    action = payload.substring(thirdDot + 1, fourthDot);
    params = payload.substring(fourthDot + 1);
  } else {
    action = payload.substring(thirdDot + 1);
  }
  
  Serial.println("DeviceID: " + deviceID + ", Function: " + function + 
                ", InstanceID: " + instanceID + ", Action: " + action +
                ", Params: " + params);
  
  // Verarbeite verschiedene Telegramm-Typen
  
  // Prüfe, ob es ein Telegramm zur Steuerung der Hintergrundbeleuchtung ist
  if (function == "LBN" && instanceID == "1" && action == "SET_MBR" && params != "") {
    int brightness = params.toInt();
    
    // Setze die Hintergrundbeleuchtung
    setBacklight(brightness);
    
    // Aktualisiere Menu-Anzeige
    tft.fillRect(0, SCREEN_HEIGHT - 20, 200, 20, TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("Helligkeit: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 20, 1);
    
    Serial.println("Helligkeit gesetzt auf: " + String(brightness) + "%");
  }
  
  // Prüfe, ob es ein Telegramm zur Steuerung der Button-Beleuchtung ist
  else if (function == "LED" && params != "") {
    int buttonIndex = findButtonByInstanceID(instanceID);
    
    if (buttonIndex >= 0) {
      int status = params.toInt();
      
      // Setze den Button-Status entsprechend
      setButtonActive(buttonIndex, status == 1);
      
      Serial.print("Button ");
      Serial.print(instanceID);
      Serial.print(" Beleuchtung ");
      Serial.println(status == 1 ? "EIN" : "AUS");
    } else {
      Serial.println("Button mit ID " + instanceID + " nicht gefunden");
    }
  }
  
  // Hier könnten weitere Telegramm-Typen verarbeitet werden
}

// Funktion zum Lesen und Verarbeiten der empfangenen Telegramme byteweise
void processIncomingTelegrams() {
  // Verarbeite Bytes von USB Serial
  processSerialBytes(Serial);
  
  // Verarbeite Bytes von Hardware Serial
  processSerialBytes(SerialPort);
}