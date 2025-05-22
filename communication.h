/**
 * communication.h - Erweitert für CSMA/CD
 * 
 * Neue Funktionen für Kollisionsvermeidung und Sendepuffer-Management
 */
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "config.h"
#include <HardwareSerial.h>

/**
 * Initialisiert die CSMA/CD-Kommunikation
 */
void setupCommunication();

/**
 * Sendet ein Telegramm mit automatischer Prioritätszuweisung
 * Das Telegramm wird in den Sendepuffer eingereiht und bei freiem Bus gesendet
 * 
 * @param function     Funktionskategorie (z.B. "BTN", "LED", "BLT")
 * @param instanceID   ID der Instanz (z.B. "17", "18")
 * @param action       Aktionsbezeichnung (z.B. "STATUS", "SET")
 * @param params       Optionale Parameter (Standard: "")
 */
void sendTelegram(String function, String instanceID, String action, String params = "");

/**
 * Hauptupdate-Funktion für die Kommunikation
 * Muss regelmäßig in der loop() aufgerufen werden
 * - Verarbeitet Sendepuffer
 * - Empfängt eingehende Telegramme
 * - Überwacht Bus-Status
 */
void updateCommunication();

/**
 * Prüft, ob der RS485-Bus frei ist (Carrier Sense)
 * 
 * @return true wenn der Bus frei ist, false wenn belegt
 */
bool isBusIdle();

/**
 * Sendet ein Telegramm direkt mit CSMA/CD-Algorithmus
 * Nur für interne Verwendung - normalerweise sendTelegram() verwenden
 * 
 * @param telegram     Das komplette Telegramm als String
 * @param maxRetries   Maximale Anzahl der Wiederholungsversuche
 * @return true bei erfolgreichem Senden, false bei Fehlschlag
 */
bool transmitWithCSMA(const String& telegram, int maxRetries = 3);

/**
 * Verarbeitet empfangene Telegramme
 * Wird automatisch von updateCommunication() aufgerufen
 */
void processIncomingTelegrams();

/**
 * Verarbeitet den Sendepuffer
 * Wird automatisch von updateCommunication() aufgerufen
 */
void processSendQueue();

/**
 * Gibt Kommunikations-Statistiken aus
 * Zeigt Anzahl gesendeter Telegramme, Kollisionen, etc.
 */
void printCommunicationStats();

/**
 * Verarbeitet ein empfangenes Telegramm
 * Prüft das Format und führt die entsprechende Aktion aus
 * 
 * @param telegramStr  Das zu verarbeitende Telegramm als String
 */
void processTelegram(String telegramStr);

/**
 * Gibt ein Telegramm in hexadezimaler Form aus (für Debugging)
 * 
 * @param telegram     Das auszugebende Telegramm
 */
void printTelegramHex(String telegram);

/**
 * Gibt alle empfangenen Rohdaten aus (für Diagnose)
 * Diese Funktion sollte nur für Debugging verwendet werden
 */
void dumpRawData();

/**
 * Erweiterte Sendefunktion mit Priorität und Dringlichkeit
 * 
 * @param function     Funktionskategorie
 * @param instanceID   Instanz-ID
 * @param action       Aktion
 * @param params       Parameter
 * @param priority     Priorität (0=höchste, 9=niedrigste)
 * @param urgent       Dringend (überspringt Warteschlange)
 */
void sendTelegramWithPriority(String function, String instanceID, String action, 
                              String params = "", int priority = 5, bool urgent = false);

/**
 * Berechnet die Backoff-Zeit bei Kollisionen
 * Implementiert exponentielles Backoff mit Zufallskomponente
 * 
 * @param retryCount   Anzahl der bisherigen Wiederholungsversuche
 * @return Wartezeit in Millisekunden
 */
unsigned long calculateBackoffTime(int retryCount);

/**
 * Erkennt Kollisionen während des Sendens
 * Vergleicht gesendete mit empfangenen Daten
 * 
 * @param sentTelegram Das gesendete Telegramm
 * @return true bei erkannter Kollision, false wenn OK
 */
bool detectCollision(const String& sentTelegram);

/**
 * Fügt ein Telegramm zum Sendepuffer hinzu
 * 
 * @param telegram     Das komplette Telegramm
 * @param priority     Priorität (0-9)
 * @param urgent       Dringlichkeits-Flag
 * @return true bei Erfolg, false wenn Puffer voll
 */
bool addToSendQueue(const String& telegram, int priority = 5, bool urgent = false);

/**
 * Holt das nächste Telegramm aus dem Sendepuffer
 * Berücksichtigt Prioritäten und Dringlichkeit
 * 
 * @param item         Referenz auf SendQueueItem-Struktur
 * @return true wenn Telegramm verfügbar, false wenn Puffer leer
 */
bool getNextFromSendQueue(struct SendQueueItem& item);

/**
 * Leert den Sendepuffer (für Notfälle)
 */
void clearSendQueue();

/**
 * Gibt die aktuelle Sendepuffer-Auslastung zurück
 * 
 * @return Anzahl der wartenden Telegramme
 */
int getSendQueueCount();

/**
 * Setzt Kommunikations-Statistiken zurück
 */
void resetCommunicationStats();

// Externe Variablen für Statistiken
extern unsigned long totalSent;
extern unsigned long totalCollisions;
extern unsigned long totalRetries;

// Konstanten für CSMA/CD-Timing
extern const unsigned long BUS_IDLE_TIME;
extern const unsigned long COLLISION_DETECT_TIME;

#endif // COMMUNICATION_H