/**
 * web_interface.h - Version 1.50
 * 
 * Web-Interface Funktions-Deklarationen für ESP32 Service-Manager
 */

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include "Arduino.h"

// Web-Server Setup und Verwaltung
void setupWebServer();

// HTML-Seiten Generator
String getMainPage();
String getCSS();
String getRestartPage();
String getWiFiOffPage();
String get404Page();

// API und Formular-Handler
void handleSaveConfig();
String getStatusJSON();

#endif // WEB_INTERFACE_H