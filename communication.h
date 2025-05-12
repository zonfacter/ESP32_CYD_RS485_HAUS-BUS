// communication.h
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "config.h"
#include <HardwareSerial.h>

// Externe Hardware-Serial-Instanz für die UART-Schnittstelle
extern HardwareSerial SerialPort;

// Puffer für eingehende Telegramme
#define MAX_TELEGRAM_LENGTH 100
extern char telegramBuffer[MAX_TELEGRAM_LENGTH];
extern int bufferPos;
extern bool receivingTelegram;

// Initialisiert die Kommunikation (serielle Schnittstellen)
void setupCommunication();

// Funktion zum Senden eines Protokoll-Telegramms
void sendTelegram(String function, String instanceID, String action, String params = "");

// Hilfsfunktion zur HEX-Ausgabe von Telegrammen
void printTelegramHex(String telegram);

// Hilfsfunktion zur Verarbeitung eines Telegramms
void processTelegram(String telegramStr);

// Funktion zum Lesen und Verarbeiten der empfangenen Telegramme
void processIncomingTelegrams();

// Verarbeitet Bytes von einer seriellen Schnittstelle
void processSerialBytes(Stream &serialPort);

#endif // COMMUNICATION_H