#ifndef CONVERTER_WEB_SERVICE_H
#define CONVERTER_WEB_SERVICE_H

#include "Arduino.h"
#include "ArduinoJson.h"

// *** FALLBACK: NUM_BUTTONS definieren falls nicht verfügbar ***
#ifndef NUM_BUTTONS
#define NUM_BUTTONS 6  // Standard-Wert als Fallback
#endif

// ===== DATEN-STRUKTUREN =====

// Einheitliche Button-Konfiguration
struct ButtonData {
  String label;
  String instanceID;
  uint16_t colorInactive;
  uint16_t colorActive;
  uint16_t textColor;
  bool enabled;
  bool isActive;
  uint8_t priority;
  String iconType;
};

// System-Konfiguration
struct SystemData {
  String deviceID;
  int orientation;
  int brightness;
  bool debugMode;
  String firmwareVersion;
  unsigned long uptime;
};

// Netzwerk-Konfiguration
struct NetworkData {
  String apSSID;
  String apPassword;
  bool wifiActive;
  bool webServerActive;
  String ipAddress;
};

// ===== CONVERTER WEB SERVICE KLASSE =====

class ConverterWebService {
private:
  // Daten-Cache
  ButtonData buttonCache[NUM_BUTTONS];  // ← Jetzt sollte es funktionieren
  SystemData systemCache;
  NetworkData networkCache;
  
  // Hilfsfunktionen
  uint16_t hexToColor565(String hexColor);
  String color565ToHex(uint16_t color565);
  bool validateButtonData(const ButtonData& data);
  bool validateSystemData(const SystemData& data);
  
  // Speicher-Operationen
  bool saveToEEPROM();
  bool loadFromEEPROM();
  bool saveToSPIFFS();
  bool loadFromSPIFFS();

public:
  ConverterWebService();
  
  // ===== INITIALISIERUNG =====
  bool begin();
  void end();
  
  // ===== BUTTON-KONVERTIERUNG =====
  
  // Web → System
  bool setButtonsFromJSON(const String& jsonData);
  bool setButtonFromWeb(int buttonIndex, const String& label, 
                       const String& instanceID, const String& colorHex = "#808080");
  
  // System → Web
  String getButtonsAsJSON();
  String getButtonAsJSON(int buttonIndex);
  
  // System → Display
  bool applyButtonsToDisplay();
  bool applyButtonToDisplay(int buttonIndex);
  
  // Display → System
  bool syncButtonsFromDisplay();
  
  // ===== SYSTEM-KONVERTIERUNG =====
  
  // Web → System
  bool setSystemFromJSON(const String& jsonData);
  bool setSystemConfig(const String& deviceID, int orientation, 
                      int brightness, bool debugMode = false);
  
  // System → Web
  String getSystemAsJSON();
  
  // System → Service Manager
  bool applySystemToServiceManager();
  
  // Service Manager → System
  bool syncSystemFromServiceManager();
  
  // ===== NETZWERK-KONVERTIERUNG =====
  
  // System → Web
  String getNetworkAsJSON();
  
  // Aktualisierung
  bool updateNetworkStatus();
  
  // ===== PERSISTENTE SPEICHERUNG =====
  
  // Alles speichern
  bool saveAll();
  bool loadAll();
  
  // Einzeln speichern
  bool saveButtons();
  bool saveSystem();
  bool loadButtons();
  bool loadSystem();
  
  // ===== VALIDIERUNG & DIAGNOSE =====
  
  bool validateAllData();
  String getDiagnosticInfo();
  void printStatus();
  
  // ===== EVENT-SYSTEM =====
  
  typedef void (*ConfigChangedCallback)(String configType);
  void setConfigChangedCallback(ConfigChangedCallback callback);
  
  // ===== TEMPLATE-SYSTEM =====
  
  bool applyTemplate(const String& templateName);
  String getAvailableTemplates();
  bool saveAsTemplate(const String& templateName);
  
private:
  ConfigChangedCallback configCallback;
  void triggerConfigChanged(String configType);
};

// Globale Instanz
extern ConverterWebService webConverter;

// ===== HILFSFUNKTIONEN =====

// Template-Definitionen
struct ButtonTemplate {
  const char* name;
  const char* labels[NUM_BUTTONS];  // ← Jetzt sollte es funktionieren
};

extern const ButtonTemplate TEMPLATES[];
extern const int TEMPLATE_COUNT;

#endif // CONVERTER_WEB_SERVICE_H