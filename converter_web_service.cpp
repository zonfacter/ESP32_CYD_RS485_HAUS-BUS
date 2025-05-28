// ===== IMPLEMENTIERUNG converter_web_service.cpp =====

#include "converter_web_service.h"
#include "service_manager.h"
#include "menu.h"
#include "web_server_manager.h"
#include <EEPROM.h>
#include <SPIFFS.h>

// Globale Instanz
ConverterWebService webConverter;

// Template-Definitionen
const ButtonTemplate TEMPLATES[] = {
  {"wohnzimmer", {"Deckenlampe", "Stehlampe", "TV", "Rollade", "Heizung", "Lüftung"}},
  {"schlafzimmer", {"Hauptlicht", "Nachttisch", "Rollade", "Heizung", "Lüftung", "Alarm"}},
  {"küche", {"Arbeitsplatte", "Spüle", "Herd", "Dunstabzug", "Fenster", "Radio"}},
  {"büro", {"Schreibtisch", "Monitor", "Drucker", "Jalousie", "Klima", "Musik"}},
  {"bad", {"Spiegel", "Dusche", "Lüftung", "Heizung", "Fenster", "Nachtlicht"}},
  {"rolladen", {"Wohnen Auf", "Wohnen Ab", "Küche Auf", "Küche Ab", "Alle Auf", "Alle Ab"}}
};

const int TEMPLATE_COUNT = sizeof(TEMPLATES) / sizeof(ButtonTemplate);

// EEPROM Adressen
#define CONVERTER_EEPROM_START 200
#define CONVERTER_MAGIC_BYTE 0xC5

struct ConverterEEPROMData {
  uint8_t magic;
  ButtonData buttons[NUM_BUTTONS];
  SystemData system;
  uint8_t checksum;
};

// ===== KONSTRUKTOR & INITIALISIERUNG =====

ConverterWebService::ConverterWebService() {
  configCallback = nullptr;
  
  // Standard-Werte setzen
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonCache[i].label = "Taster " + String(i + 1);
    buttonCache[i].instanceID = String(17 + i);
    buttonCache[i].colorInactive = TFT_DARKGREY;
    buttonCache[i].colorActive = TFT_GREEN;
    buttonCache[i].textColor = TFT_WHITE;
    buttonCache[i].enabled = true;
    buttonCache[i].isActive = false;
    buttonCache[i].priority = 5;
    buttonCache[i].iconType = "SWITCH";
  }
  
  systemCache.deviceID = "5999";
  systemCache.orientation = 0;
  systemCache.brightness = 100;
  systemCache.debugMode = false;
  systemCache.firmwareVersion = "1.60";
  systemCache.uptime = 0;
}

bool ConverterWebService::begin() {
  Serial.println("🔄 Converter Web Service wird initialisiert...");
  
  // EEPROM initialisieren
  EEPROM.begin(512);
  
  // SPIFFS prüfen
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ SPIFFS Mount fehlgeschlagen");
    return false;
  }
  
  // Konfiguration laden
  if (!loadAll()) {
    Serial.println("⚠️ Keine gespeicherte Konfiguration gefunden - verwende Standards");
  }
  
  Serial.println("✅ Converter Web Service initialisiert");
  printStatus();
  
  return true;
}

// ===== BUTTON-KONVERTIERUNG =====

bool ConverterWebService::setButtonsFromJSON(const String& jsonData) {
  Serial.println("📥 Konvertiere Button-JSON zu System-Daten");
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
    return false;
  }
  
  // Buttons aus JSON extrahieren
  JsonArray buttonsArray = doc["buttons"];
  int buttonCount = 0;
  
  for (JsonObject btn : buttonsArray) {
    if (buttonCount >= NUM_BUTTONS) break;
    
    buttonCache[buttonCount].label = btn["label"] | buttonCache[buttonCount].label;
    buttonCache[buttonCount].instanceID = btn["instanceID"] | buttonCache[buttonCount].instanceID;
    
    if (btn.containsKey("colorInactive")) {
      buttonCache[buttonCount].colorInactive = hexToColor565(btn["colorInactive"]);
    }
    
    if (btn.containsKey("textColor")) {
      buttonCache[buttonCount].textColor = hexToColor565(btn["textColor"]);
    }
    
    buttonCache[buttonCount].enabled = btn["enabled"] | true;
    buttonCache[buttonCount].priority = btn["priority"] | 5;
    
    Serial.printf("  Button %d: '%s' (ID: %s)\n", 
                 buttonCount + 1, 
                 buttonCache[buttonCount].label.c_str(),
                 buttonCache[buttonCount].instanceID.c_str());
    
    buttonCount++;
  }
  
  // Auf Display anwenden
  applyButtonsToDisplay();
  
  // Speichern
  saveButtons();
  
  triggerConfigChanged("buttons");
  
  Serial.println("✅ Button-Konfiguration erfolgreich konvertiert und angewendet");
  return true;
}

String ConverterWebService::getButtonsAsJSON() {
  // Aktuelle Display-Buttons zu JSON konvertieren
  syncButtonsFromDisplay();
  
  DynamicJsonDocument doc(2048);
  JsonArray buttonsArray = doc.createNestedArray("buttons");
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    JsonObject btn = buttonsArray.createNestedObject();
    btn["id"] = i;
    btn["label"] = buttonCache[i].label;
    btn["instanceID"] = buttonCache[i].instanceID;
    btn["colorInactive"] = color565ToHex(buttonCache[i].colorInactive);
    btn["colorActive"] = color565ToHex(buttonCache[i].colorActive);
    btn["textColor"] = color565ToHex(buttonCache[i].textColor);
    btn["enabled"] = buttonCache[i].enabled;
    btn["isActive"] = buttonCache[i].isActive;
    btn["priority"] = buttonCache[i].priority;
    btn["iconType"] = buttonCache[i].iconType;
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool ConverterWebService::applyButtonsToDisplay() {
  Serial.println("🎨 Wende Button-Konfiguration auf Display an");
  
  // Sicherstellen, dass buttons[] Array existiert (aus menu.cpp)
  extern Button buttons[NUM_BUTTONS];
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].label = buttonCache[i].label;
    buttons[i].instanceID = buttonCache[i].instanceID;
    buttons[i].color = buttonCache[i].colorInactive;
    buttons[i].textColor = buttonCache[i].textColor;
    buttons[i].isActive = buttonCache[i].isActive;
    
    Serial.printf("  Button %d: '%s' → Display\n", 
                 i + 1, buttons[i].label.c_str());
  }
  
  // Display neu zeichnen
  drawButtons();
  
  Serial.println("✅ Button-Konfiguration auf Display angewendet");
  return true;
}

bool ConverterWebService::syncButtonsFromDisplay() {
  // Display-Buttons zu Cache synchronisieren
  extern Button buttons[NUM_BUTTONS];
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonCache[i].label = buttons[i].label;
    buttonCache[i].instanceID = buttons[i].instanceID;
    buttonCache[i].colorInactive = buttons[i].color;
    buttonCache[i].textColor = buttons[i].textColor;
    buttonCache[i].isActive = buttons[i].isActive;
  }
  
  return true;
}

// ===== TEMPLATE-SYSTEM =====

bool ConverterWebService::applyTemplate(const String& templateName) {
  Serial.println("📋 Wende Template an: " + templateName);
  
  // Template finden
  const ButtonTemplate* selectedTemplate = nullptr;
  for (int i = 0; i < TEMPLATE_COUNT; i++) {
    if (templateName == TEMPLATES[i].name) {
      selectedTemplate = &TEMPLATES[i];
      break;
    }
  }
  
  if (!selectedTemplate) {
    Serial.println("❌ Template nicht gefunden: " + templateName);
    return false;
  }
  
  // Template anwenden
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonCache[i].label = String(selectedTemplate->labels[i]);
    buttonCache[i].instanceID = String(17 + i);  // Standard IDs
    buttonCache[i].colorInactive = TFT_DARKGREY;
    buttonCache[i].textColor = TFT_WHITE;
    buttonCache[i].enabled = true;
    buttonCache[i].isActive = false;
    
    Serial.printf("  Button %d: %s\n", i + 1, selectedTemplate->labels[i]);
  }
  
  // Auf Display anwenden
  applyButtonsToDisplay();
  
  // Speichern
  saveButtons();
  
  triggerConfigChanged("buttons");
  
  Serial.println("✅ Template '" + templateName + "' erfolgreich angewendet");
  return true;
}

String ConverterWebService::getAvailableTemplates() {
  DynamicJsonDocument doc(1024);
  JsonArray templatesArray = doc.createNestedArray("templates");
  
  for (int i = 0; i < TEMPLATE_COUNT; i++) {
    JsonObject tmpl = templatesArray.createNestedObject();
    tmpl["name"] = TEMPLATES[i].name;
    tmpl["displayName"] = String(TEMPLATES[i].name) + " Template";
    
    JsonArray labelsArray = tmpl.createNestedArray("labels");
    for (int j = 0; j < NUM_BUTTONS; j++) {
      labelsArray.add(TEMPLATES[i].labels[j]);
    }
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

// ===== SYSTEM-KONVERTIERUNG =====

String ConverterWebService::getSystemAsJSON() {
  // Aktuelle System-Daten vom Service Manager holen
  syncSystemFromServiceManager();
  
  DynamicJsonDocument doc(1024);
  doc["deviceID"] = systemCache.deviceID;
  doc["orientation"] = systemCache.orientation;
  doc["brightness"] = systemCache.brightness;
  doc["debugMode"] = systemCache.debugMode;
  doc["firmwareVersion"] = systemCache.firmwareVersion;
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool ConverterWebService::syncSystemFromServiceManager() {
  systemCache.deviceID = serviceManager.getDeviceID();
  systemCache.orientation = serviceManager.getOrientation();
  systemCache.brightness = currentBacklight;
  systemCache.uptime = millis() / 1000;
  
  return true;
}

bool ConverterWebService::applySystemToServiceManager() {
  Serial.println("⚙️ Wende System-Konfiguration auf Service Manager an");
  
  serviceManager.setDeviceID(systemCache.deviceID);
  serviceManager.setOrientation(systemCache.orientation);
  setBacklight(systemCache.brightness);
  
  Serial.println("✅ System-Konfiguration angewendet");
  return true;
}

// ===== PERSISTENTE SPEICHERUNG =====

bool ConverterWebService::saveAll() {
  Serial.println("💾 Speichere komplette Konfiguration");
  
  bool success = true;
  success &= saveButtons();
  success &= saveSystem();
  
  if (success) {
    Serial.println("✅ Konfiguration erfolgreich gespeichert");
  } else {
    Serial.println("❌ Fehler beim Speichern der Konfiguration");
  }
  
  return success;
}

bool ConverterWebService::saveButtons() {
  // Sync von Display
  syncButtonsFromDisplay();
  
  // In EEPROM speichern
  ConverterEEPROMData data;
  data.magic = CONVERTER_MAGIC_BYTE;
  
  // Button-Daten kopieren
  for (int i = 0; i < NUM_BUTTONS; i++) {
    data.buttons[i] = buttonCache[i];
  }
  
  // System-Daten kopieren
  data.system = systemCache;
  
  // Checksum berechnen
  uint8_t checksum = data.magic;
  uint8_t* ptr = (uint8_t*)&data.buttons[0];
  for (size_t i = 0; i < sizeof(data.buttons) + sizeof(data.system); i++) {
    checksum += ptr[i];
  }
  data.checksum = checksum;
  
  // In EEPROM schreiben
  EEPROM.put(CONVERTER_EEPROM_START, data);
  EEPROM.commit();
  
  Serial.println("💾 Button-Konfiguration in EEPROM gespeichert");
  return true;
}

bool ConverterWebService::loadButtons() {
  ConverterEEPROMData data;
  EEPROM.get(CONVERTER_EEPROM_START, data);
  
  // Validierung
  if (data.magic != CONVERTER_MAGIC_BYTE) {
    Serial.println("⚠️ Keine gültige Button-Konfiguration in EEPROM gefunden");
    return false;
  }
  
  // Checksum prüfen
  uint8_t checksum = data.magic;
  uint8_t* ptr = (uint8_t*)&data.buttons[0];
  for (size_t i = 0; i < sizeof(data.buttons) + sizeof(data.system); i++) {
    checksum += ptr[i];
  }
  
  if (checksum != data.checksum) {
    Serial.println("❌ Button-Konfiguration Checksum-Fehler");
    return false;
  }
  
  // Daten laden
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonCache[i] = data.buttons[i];
  }
  systemCache = data.system;
  
  Serial.println("📥 Button-Konfiguration aus EEPROM geladen");
  return true;
}

bool ConverterWebService::loadAll() {
  bool success = loadButtons();
  
  if (success) {
    // Auf Display anwenden
    applyButtonsToDisplay();
    applySystemToServiceManager();
  }
  
  return success;
}

// ===== HILFSFUNKTIONEN =====

uint16_t ConverterWebService::hexToColor565(String hexColor) {
  if (hexColor.startsWith("#")) {
    hexColor = hexColor.substring(1);
  }
  
  if (hexColor.length() != 6) {
    return TFT_DARKGREY;
  }
  
  long hexValue = strtol(hexColor.c_str(), NULL, 16);
  uint8_t r = (hexValue >> 16) & 0xFF;
  uint8_t g = (hexValue >> 8) & 0xFF;
  uint8_t b = hexValue & 0xFF;
  
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

String ConverterWebService::color565ToHex(uint16_t color565) {
  uint8_t r = (color565 >> 8) & 0xF8;
  uint8_t g = (color565 >> 3) & 0xFC;
  uint8_t b = (color565 << 3) & 0xF8;
  
  char hexString[8];
  sprintf(hexString, "#%02X%02X%02X", r, g, b);
  return String(hexString);
}

void ConverterWebService::printStatus() {
  Serial.println("\n📊 Converter Web Service Status:");
  Serial.println("═══════════════════════════════════");
  
  Serial.println("🔘 Button-Konfiguration:");
  for (int i = 0; i < NUM_BUTTONS; i++) {
    Serial.printf("  %d: '%s' (ID: %s) %s\n", 
                 i + 1, 
                 buttonCache[i].label.c_str(),
                 buttonCache[i].instanceID.c_str(),
                 color565ToHex(buttonCache[i].colorInactive).c_str());
  }
  
  Serial.println("\n⚙️ System-Konfiguration:");
  Serial.println("  Device ID: " + systemCache.deviceID);
  Serial.println("  Orientierung: " + String(systemCache.orientation == 0 ? "Portrait" : "Landscape"));
  Serial.println("  Helligkeit: " + String(systemCache.brightness) + "%");
  Serial.println("  Firmware: " + systemCache.firmwareVersion);
  
  Serial.println("═══════════════════════════════════\n");
}

void ConverterWebService::triggerConfigChanged(String configType) {
  if (configCallback) {
    configCallback(configType);
  }
}

void ConverterWebService::setConfigChangedCallback(ConfigChangedCallback callback) {
  configCallback = callback;
}

// ===== FEHLENDE FUNKTIONEN IN converter_web_service.cpp =====

bool ConverterWebService::saveSystem() {
  // System-Daten vom Service Manager synchronisieren
  syncSystemFromServiceManager();
  
  // System-Daten werden zusammen mit Buttons in saveButtons() gespeichert
  return true;
}

bool ConverterWebService::loadSystem() {
  // System-Daten werden zusammen mit Buttons in loadButtons() geladen
  return true;
}

bool ConverterWebService::saveToEEPROM() {
  // Wird in saveButtons() implementiert
  return saveButtons();
}

bool ConverterWebService::loadFromEEPROM() {
  // Wird in loadButtons() implementiert
  return loadButtons();
}

bool ConverterWebService::saveToSPIFFS() {
  // Optional: Zusätzliche SPIFFS-Speicherung
  return true;
}

bool ConverterWebService::loadFromSPIFFS() {
  // Optional: SPIFFS-Laden
  return true;
}

bool ConverterWebService::validateButtonData(const ButtonData& data) {
  // Validierung der Button-Daten
  if (data.label.length() == 0 || data.label.length() > 15) {
    return false;
  }
  
  if (data.instanceID.length() != 2 && data.instanceID.length() != 3) {
    return false;
  }
  
  if (data.priority > 9) {
    return false;
  }
  
  return true;
}

bool ConverterWebService::validateSystemData(const SystemData& data) {
  // Validierung der System-Daten
  if (data.deviceID.length() != 4) {
    return false;
  }
  
  if (data.orientation < 0 || data.orientation > 3) {
    return false;
  }
  
  if (data.brightness < 0 || data.brightness > 100) {
    return false;
  }
  
  return true;
}

bool ConverterWebService::validateAllData() {
  // Alle Button-Daten validieren
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (!validateButtonData(buttonCache[i])) {
      return false;
    }
  }
  
  // System-Daten validieren
  if (!validateSystemData(systemCache)) {
    return false;
  }
  
  return true;
}

String ConverterWebService::getDiagnosticInfo() {
  String info = "=== Converter Web Service Diagnose ===\n";
  info += "Button Cache Status:\n";
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    info += "  Button " + String(i + 1) + ": '" + buttonCache[i].label + "' ";
    info += "(ID: " + buttonCache[i].instanceID + ")\n";
  }
  
  info += "\nSystem Cache Status:\n";
  info += "  Device ID: " + systemCache.deviceID + "\n";
  info += "  Orientation: " + String(systemCache.orientation) + "\n";
  info += "  Brightness: " + String(systemCache.brightness) + "%\n";
  
  info += "\nValidation: " + String(validateAllData() ? "OK" : "FEHLER") + "\n";
  info += "=====================================\n";
  
  return info;
}

bool ConverterWebService::setSystemFromJSON(const String& jsonData) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.println("❌ System JSON Parse Error: " + String(error.c_str()));
    return false;
  }
  
  // System-Daten aus JSON extrahieren
  systemCache.deviceID = doc["deviceID"] | systemCache.deviceID;
  systemCache.orientation = doc["orientation"] | systemCache.orientation;
  systemCache.brightness = doc["brightness"] | systemCache.brightness;
  systemCache.debugMode = doc["debugMode"] | systemCache.debugMode;
  
  // Auf Service Manager anwenden
  applySystemToServiceManager();
  
  // Speichern
  saveSystem();
  
  triggerConfigChanged("system");
  
  return true;
}

bool ConverterWebService::setSystemConfig(const String& deviceID, int orientation, 
                                         int brightness, bool debugMode) {
  systemCache.deviceID = deviceID;
  systemCache.orientation = orientation;
  systemCache.brightness = brightness;
  systemCache.debugMode = debugMode;
  
  // Validieren
  if (!validateSystemData(systemCache)) {
    Serial.println("❌ Ungültige System-Konfiguration");
    return false;
  }
  
  // Anwenden und speichern
  applySystemToServiceManager();
  saveSystem();
  
  triggerConfigChanged("system");
  
  return true;
}

bool ConverterWebService::setButtonFromWeb(int buttonIndex, const String& label, 
                                          const String& instanceID, const String& colorHex) {
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
    return false;
  }
  
  buttonCache[buttonIndex].label = label.substring(0, 15);
  buttonCache[buttonIndex].instanceID = instanceID;
  buttonCache[buttonIndex].colorInactive = hexToColor565(colorHex);
  buttonCache[buttonIndex].textColor = TFT_WHITE;
  buttonCache[buttonIndex].enabled = true;
  buttonCache[buttonIndex].priority = 5;
  
  // Auf Display anwenden
  applyButtonToDisplay(buttonIndex);
  
  // Speichern
  saveButtons();
  
  triggerConfigChanged("buttons");
  
  return true;
}

String ConverterWebService::getButtonAsJSON(int buttonIndex) {
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
    return "{}";
  }
  
  DynamicJsonDocument doc(512);
  doc["id"] = buttonIndex;
  doc["label"] = buttonCache[buttonIndex].label;
  doc["instanceID"] = buttonCache[buttonIndex].instanceID;
  doc["colorInactive"] = color565ToHex(buttonCache[buttonIndex].colorInactive);
  doc["colorActive"] = color565ToHex(buttonCache[buttonIndex].colorActive);
  doc["textColor"] = color565ToHex(buttonCache[buttonIndex].textColor);
  doc["enabled"] = buttonCache[buttonIndex].enabled;
  doc["isActive"] = buttonCache[buttonIndex].isActive;
  doc["priority"] = buttonCache[buttonIndex].priority;
  doc["iconType"] = buttonCache[buttonIndex].iconType;
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool ConverterWebService::applyButtonToDisplay(int buttonIndex) {
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
    return false;
  }
  
  extern Button buttons[NUM_BUTTONS];
  
  buttons[buttonIndex].label = buttonCache[buttonIndex].label;
  buttons[buttonIndex].instanceID = buttonCache[buttonIndex].instanceID;
  buttons[buttonIndex].color = buttonCache[buttonIndex].colorInactive;
  buttons[buttonIndex].textColor = buttonCache[buttonIndex].textColor;
  buttons[buttonIndex].isActive = buttonCache[buttonIndex].isActive;
  
  // Einzelnen Button neu zeichnen
  redrawButton(buttonIndex);
  
  return true;
}

String ConverterWebService::getNetworkAsJSON() {
  updateNetworkStatus();
  
  DynamicJsonDocument doc(512);
  doc["apSSID"] = networkCache.apSSID;
  doc["wifiActive"] = networkCache.wifiActive;
  doc["webServerActive"] = networkCache.webServerActive;
  doc["ipAddress"] = networkCache.ipAddress;
  
  String result;
  serializeJson(doc, result);
  return result;
}

bool ConverterWebService::updateNetworkStatus() {
  // WiFi-Status vom Service Manager holen
  networkCache.wifiActive = serviceManager.isWiFiActive();
  networkCache.webServerActive = serviceManager.isWebServerActive();
  networkCache.apSSID = "ESP32-ServiceMode";
  
  if (networkCache.wifiActive) {
    networkCache.ipAddress = WiFi.softAPIP().toString();
  } else {
    networkCache.ipAddress = "0.0.0.0";
  }
  
  return true;
}

bool ConverterWebService::saveAsTemplate(const String& templateName) {
  // SPIFFS-Datei erstellen
  String templatePath = "/templates/" + templateName + ".json";
  String currentConfig = getButtonsAsJSON();
  
  File file = SPIFFS.open(templatePath, "w");
  if (file) {
    file.print(currentConfig);
    file.close();
    return true;
  }
  return false;
}

void ConverterWebService::end() {
  // Cleanup falls nötig
  configCallback = nullptr;
  Serial.println("🔄 Converter Web Service beendet");
}