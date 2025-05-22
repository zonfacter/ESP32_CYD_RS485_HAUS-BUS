/**
 * web_config.h - Version 1.50
 * 
 * Separate Web-Server Funktionen um Include-Konflikte zu vermeiden
 */

#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

#include "Arduino.h"

// Forward-Deklaration
class ServiceManager;

// Web-Server Funktionen
void initWebServer();
void startWebServerForService(ServiceManager* sm);
void stopWebServerForService();
void handleWebServerClient();
bool isWebServerRunning();

#endif // WEB_CONFIG_H