/**
 * service_manager.h - Version 1.50
 * 
 * Service-Menü für ESP32 Touch-Interface
 * - 20-Sekunden Touch-Aktivierung
 * - Device ID Konfiguration
 * - Orientierungs-Umschaltung
 * - Telegramm-Steuerung
 * - WiFi Access Point
 */

#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include "config.h"

// Service-Modus Zustände
enum ServiceState {
  SERVICE_INACTIVE,      // Normal-Modus (6-Button Interface)
  SERVICE_ACTIVATING,    // 20-Sekunden Touch läuft
  SERVICE_ACTIVE,        // Service-Menü angezeigt
  SERVICE_DEVICE_ID,     // Device ID Editor
  SERVICE_ORIENTATION    // Orientierungs-Einstellung
};

// Service-Menü Touch-Buttons
struct ServiceButton {
  int x, y, w, h;
  String label;
  uint16_t color;
  uint16_t textColor;
  bool enabled;
};

// Device ID Editor Buttons
struct NumpadButton {
  int x, y, w, h;
  String label;
  char value;
  uint16_t color;
  uint16_t textColor;
};

class ServiceManager {
private:
  ServiceState currentState;
  unsigned long touchStartTime;
  bool longTouchActive;
  unsigned long lastProgressUpdate;
  
  // Service-Menü Buttons
  static const int NUM_SERVICE_BUTTONS = 6;
  ServiceButton serviceButtons[NUM_SERVICE_BUTTONS];
  
  // Device ID Editor
  static const int NUM_NUMPAD_BUTTONS = 16;  // 0-9, +, -, <, >, OK, CANCEL
  NumpadButton numpadButtons[NUM_NUMPAD_BUTTONS];
  String editDeviceID;
  int editPosition;  // 0-3 für 4-stellige ID
  
  // Aktuelle Konfiguration
  String currentDeviceID;
  int currentOrientation;
  bool configChanged;
  
  // WiFi und Web-Interface
  bool wifiActive;
  bool webServerActive;
  String wifiSSID;
  String wifiPassword;
  
  // Progress-Anzeige für 20-Sekunden-Touch
  int progressPercent;
  
  // Private Hilfsfunktionen
  void initServiceButtons();
  void initNumpadButtons();
  
  // Device ID Editor (private)
  void drawDeviceIDEditor();
  void drawNumpad();
  void drawDeviceIDDisplay();
  int checkNumpadButtonPress(int x, int y);
  void handleNumpadButtonPress(int buttonIndex);
  void updateDeviceIDDigit(char digit);
  void moveEditPosition(int direction);  // -1 links, +1 rechts
  void incrementCurrentDigit();
  void decrementCurrentDigit();
  void confirmDeviceIDEdit();
  void cancelDeviceIDEdit();
  
  // Orientierungs-Umschaltung (private)
  void toggleOrientation();
  void applyOrientation(int orientation);
  void showOrientationPreview();
  
  // UI-Funktionen (private)
  void drawServiceMenu();
  void drawProgressBar(int percent);
  void redrawServiceMenu();
  
  // Button-Handling (private)
  int checkServiceButtonPress(int x, int y);
  void handleServiceButtonPress(int buttonIndex);
  
  // Service-Button Callbacks (private)
  void onEditDeviceID();
  void onToggleOrientation();
  void onSaveAndExit();
  void onCancel();
  void onWiFiToggle();
  void onWebConfig();
  
  // WiFi und Web-Interface (private)
  void startWiFiAP();
  void stopWiFi();
  void startWebServer();
  void stopWebServer();

public:
  ServiceManager();
  
  // Haupt-Update-Funktion (in loop() aufrufen)
  void update();
  
  // Touch-Handling
  void handleTouch(int x, int y, bool touched);
  
  // Service-Modus Steuerung
  bool isServiceMode();
  void enterServiceMode();
  void exitServiceMode();
  void cancelServiceMode();
  
  // Telegramm-Interface
  void handleServiceTelegram(String action, String params);
  
  // Konfiguration (public)
  void loadConfig();
  void saveConfig();
  String getDeviceID();
  void setDeviceID(String newID);
  int getOrientation();
  void setOrientation(int orientation);
  
  // WiFi Status (public)
  bool isWiFiActive();
  bool isWebServerActive();
  void handleWebServer();  // Für loop() Aufruf
};

// Globale ServiceManager Instanz
extern ServiceManager serviceManager;

// Hilfsfunktionen
void setupServiceManager();
void updateServiceManager();
void handleServiceTouch(int x, int y, bool touched);
void handleWebServerLoop();  // NEU: Web-Server in loop() verwalten

#endif // SERVICE_MANAGER_H