/**
 * config_manager.cpp - Version 2.0
 * 
 * LittleFS-basierter Konfigurationsmanager Implementation
 */

#include "config_manager.h"
#include <Arduino.h>

// Globale ConfigManager Instanz
ConfigManager configManager;

// =====================================
// Logger Implementation
// =====================================

Logger::Logger(String path, size_t maxSize) : logPath(path), maxLogSize(maxSize) {
    // Sicherstellen dass Log-Verzeichnis existiert
    String dir = path.substring(0, path.lastIndexOf('/'));
    if (!LittleFS.exists(dir)) {
        LittleFS.mkdir(dir);
    }
}

void Logger::info(String message) {
    writeLog("INFO", message);
}

void Logger::warning(String message) {
    writeLog("WARN", message);
}

void Logger::error(String message) {
    writeLog("ERROR", message);
}

void Logger::debug(String message) {
    writeLog("DEBUG", message);
}

void Logger::communication(String direction, String telegram) {
    String message = direction + ": " + telegram;
    writeLog("COMM", message);
}

void Logger::system(String event, String details) {
    String message = event;
    if (details.length() > 0) {
        message += " - " + details;
    }
    writeLog("SYSTEM", message);
}

void Logger::writeLog(String level, String message) {
    // Prüfen ob Datei zu groß wird
    if (LittleFS.exists(logPath)) {
        File file = LittleFS.open(logPath, "r");
        if (file && file.size() > maxLogSize) {
            file.close();
            rotateLogs();
        } else if (file) {
            file.close();
        }
    }
    
    File file = LittleFS.open(logPath, "a");
    if (file) {
        String logEntry = getTimestamp() + " [" + level + "] " + message + "\n";
        file.print(logEntry);
        file.close();
        
        // Auch auf Serial ausgeben wenn Debug-Modus aktiv
        if (configManager.device.debugMode) {
            Serial.print(logEntry);
        }
    }
}

String Logger::getTimestamp() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    ms %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    char timestamp[32];
    sprintf(timestamp, "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, ms);
    return String(timestamp);
}

String Logger::readLogs(int lines) {
    if (!LittleFS.exists(logPath)) {
        return "Keine Logs verfügbar\n";
    }
    
    File file = LittleFS.open(logPath, "r");
    if (!file) {
        return "Fehler beim Öffnen der Log-Datei\n";
    }
    
    String content = file.readString();
    file.close();
    
    // Nur die letzten 'lines' Zeilen zurückgeben
    if (lines > 0) {
        int lineCount = 0;
        int pos = content.length() - 1;
        
        // Von hinten zählen
        while (pos >= 0 && lineCount < lines) {
            if (content.charAt(pos) == '\n') {
                lineCount++;
            }
            pos--;
        }
        
        if (pos >= 0) {
            content = content.substring(pos + 2); // +2 um \n zu überspringen
        }
    }
    
    return content;
}

void Logger::clearLogs() {
    if (LittleFS.exists(logPath)) {
        LittleFS.remove(logPath);
    }
}

void Logger::rotateLogs() {
    if (!LittleFS.exists(logPath)) return;
    
    // Alte Log-Datei umbenennen
    String backupPath = logPath + ".old";
    if (LittleFS.exists(backupPath)) {
        LittleFS.remove(backupPath);
    }
    
    LittleFS.rename(logPath, backupPath);
}

// =====================================
// ConfigManager Implementation
// =====================================

ConfigManager::ConfigManager() {
    filesystemMounted = false;
    systemLogger = nullptr;
    commLogger = nullptr;
    errorLogger = nullptr;
    configChangedCallback = nullptr;
}

ConfigManager::~ConfigManager() {
    end();
}

bool ConfigManager::begin() {
    // LittleFS initialisieren
    if (!LittleFS.begin(true)) { // true = formatOnFail
        Serial.println("ERROR: LittleFS Mount Failed");
        return false;
    }
    
    filesystemMounted = true;
    
    // Verzeichnisstruktur erstellen
    if (!createDirectoryStructure()) {
        Serial.println("ERROR: Could not create directory structure");
        return false;
    }
    
    // Logger initialisieren
    systemLogger = new Logger(LOG_SYSTEM_PATH, 50000);
    commLogger = new Logger(LOG_COMM_PATH, 30000);
    errorLogger = new Logger(LOG_ERROR_PATH, 20000);
    
    // Standard-Konfiguration setzen
    setDefaultDeviceConfig();
    setDefaultNetworkConfig();
    setDefaultDisplayConfig();
    setDefaultButtonsConfig();
    setDefaultCSMAConfig();
    
    // Konfigurationen laden oder Standard-Dateien erstellen
    if (!loadAllConfigs()) {
        LOG_WARNING("Could not load all configs, creating defaults");
        if (!createDefaultConfig()) {
            LOG_ERROR("Could not create default config files");
            return false;
        }
    }
    
    LOG_SYSTEM("ConfigManager initialized", "LittleFS mounted successfully");
    return true;
}

void ConfigManager::end() {
    if (systemLogger) {
        delete systemLogger;
        systemLogger = nullptr;
    }
    if (commLogger) {
        delete commLogger;
        commLogger = nullptr;
    }
    if (errorLogger) {
        delete errorLogger;
        errorLogger = nullptr;
    }
    
    if (filesystemMounted) {
        LittleFS.end();
        filesystemMounted = false;
    }
}

bool ConfigManager::createDirectoryStructure() {
    const char* directories[] = {
        "/config",
        "/web", 
        "/logs",
        "/backup"
    };
    
    for (const char* dir : directories) {
        if (!LittleFS.exists(dir)) {
            if (!LittleFS.mkdir(dir)) {
                Serial.printf("ERROR: Could not create directory %s\n", dir);
                return false;
            }
        }
    }
    
    return true;
}

bool ConfigManager::loadAllConfigs() {
    bool success = true;
    
    success &= loadDeviceConfig();
    success &= loadNetworkConfig();
    success &= loadDisplayConfig();
    success &= loadButtonsConfig();
    success &= loadCSMAConfig();
    
    return success;
}

bool ConfigManager::saveAllConfigs() {
    bool success = true;
    
    success &= saveDeviceConfig();
    success &= saveNetworkConfig();
    success &= saveDisplayConfig();
    success &= saveButtonsConfig();
    success &= saveCSMAConfig();
    
    if (success) {
        LOG_SYSTEM("All configurations saved successfully", "");
    } else {
        LOG_ERROR("Failed to save some configurations");
    }
    
    return success;
}

bool ConfigManager::loadDeviceConfig() {
    if (!LittleFS.exists(CONFIG_DEVICE_PATH)) {
        return false;
    }
    
    File file = LittleFS.open(CONFIG_DEVICE_PATH, "r");
    if (!file) {
        LOG_ERROR("Could not open device config file");
        return false;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("Device config JSON parse error: " + String(error.c_str()));
        return false;
    }
    
    // Konfiguration aus JSON laden
    device.deviceID = doc["deviceID"] | device.deviceID;
    device.orientation = doc["orientation"] | device.orientation;
    device.firmwareVersion = doc["firmwareVersion"] | device.firmwareVersion;
    device.hardwareVersion = doc["hardwareVersion"] | device.hardwareVersion;
    device.buildDate = doc["buildDate"] | device.buildDate;
    device.debugMode = doc["debugMode"] | device.debugMode;
    
    LOG_INFO("Device config loaded: ID=" + device.deviceID);
    return true;
}

bool ConfigManager::saveDeviceConfig() {
    DynamicJsonDocument doc(1024);
    
    doc["deviceID"] = device.deviceID;
    doc["orientation"] = device.orientation;
    doc["firmwareVersion"] = device.firmwareVersion;
    doc["hardwareVersion"] = device.hardwareVersion;
    doc["buildDate"] = device.buildDate;
    doc["uptime"] = millis() / 1000;
    doc["debugMode"] = device.debugMode;
    
    File file = LittleFS.open(CONFIG_DEVICE_PATH, "w");
    if (!file) {
        LOG_ERROR("Could not create device config file");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    LOG_INFO("Device config saved");
    
    if (configChangedCallback) {
        configChangedCallback("device");
    }
    
    return true;
}

bool ConfigManager::loadNetworkConfig() {
    if (!LittleFS.exists(CONFIG_NETWORK_PATH)) {
        return false;
    }
    
    File file = LittleFS.open(CONFIG_NETWORK_PATH, "r");
    if (!file) {
        return false;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("Network config JSON parse error");
        return false;
    }
    
    network.apSSID = doc["apSSID"] | network.apSSID;
    network.apPassword = doc["apPassword"] | network.apPassword;
    network.apEnabled = doc["apEnabled"] | network.apEnabled;
    network.staSSID = doc["staSSID"] | network.staSSID;
    network.staPassword = doc["staPassword"] | network.staPassword;
    network.staEnabled = doc["staEnabled"] | network.staEnabled;
    network.hostname = doc["hostname"] | network.hostname;
    network.dhcpEnabled = doc["dhcpEnabled"] | network.dhcpEnabled;
    network.webServerPort = doc["webServerPort"] | network.webServerPort;
    network.webServerEnabled = doc["webServerEnabled"] | network.webServerEnabled;
    
    LOG_INFO("Network config loaded");
    return true;
}

bool ConfigManager::saveNetworkConfig() {
    DynamicJsonDocument doc(1024);
    
    doc["apSSID"] = network.apSSID;
    doc["apPassword"] = network.apPassword;
    doc["apEnabled"] = network.apEnabled;
    doc["staSSID"] = network.staSSID;
    doc["staPassword"] = network.staPassword;
    doc["staEnabled"] = network.staEnabled;
    doc["hostname"] = network.hostname;
    doc["dhcpEnabled"] = network.dhcpEnabled;
    doc["webServerPort"] = network.webServerPort;
    doc["webServerEnabled"] = network.webServerEnabled;
    
    File file = LittleFS.open(CONFIG_NETWORK_PATH, "w");
    if (!file) {
        LOG_ERROR("Could not create network config file");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    LOG_INFO("Network config saved");
    
    if (configChangedCallback) {
        configChangedCallback("network");
    }
    
    return true;
}

bool ConfigManager::loadDisplayConfig() {
    if (!LittleFS.exists(CONFIG_DISPLAY_PATH)) {
        return false;
    }
    
    File file = LittleFS.open(CONFIG_DISPLAY_PATH, "r");
    if (!file) {
        return false;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("Display config JSON parse error");
        return false;
    }
    
    display.brightness = doc["brightness"] | display.brightness;
    display.statusInterval = doc["statusInterval"] | display.statusInterval;
    display.touchInvertX = doc["touchInvertX"] | display.touchInvertX;
    display.touchInvertY = doc["touchInvertY"] | display.touchInvertY;
    display.touchMinX = doc["touchMinX"] | display.touchMinX;
    display.touchMaxX = doc["touchMaxX"] | display.touchMaxX;
    display.touchMinY = doc["touchMinY"] | display.touchMinY;
    display.touchMaxY = doc["touchMaxY"] | display.touchMaxY;
    display.screenTimeout = doc["screenTimeout"] | display.screenTimeout;
    display.screensaverEnabled = doc["screensaverEnabled"] | display.screensaverEnabled;
    display.theme = doc["theme"] | display.theme;
    
    LOG_INFO("Display config loaded");
    return true;
}

bool ConfigManager::saveDisplayConfig() {
    DynamicJsonDocument doc(1024);
    
    doc["brightness"] = display.brightness;
    doc["statusInterval"] = display.statusInterval;
    doc["touchInvertX"] = display.touchInvertX;
    doc["touchInvertY"] = display.touchInvertY;
    doc["touchMinX"] = display.touchMinX;
    doc["touchMaxX"] = display.touchMaxX;
    doc["touchMinY"] = display.touchMinY;
    doc["touchMaxY"] = display.touchMaxY;
    doc["screenTimeout"] = display.screenTimeout;
    doc["screensaverEnabled"] = display.screensaverEnabled;
    doc["theme"] = display.theme;
    
    File file = LittleFS.open(CONFIG_DISPLAY_PATH, "w");
    if (!file) {
        LOG_ERROR("Could not create display config file");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    LOG_INFO("Display config saved");
    
    if (configChangedCallback) {
        configChangedCallback("display");
    }
    
    return true;
}

bool ConfigManager::loadButtonsConfig() {
    if (!LittleFS.exists(CONFIG_BUTTONS_PATH)) {
        return false;
    }
    
    File file = LittleFS.open(CONFIG_BUTTONS_PATH, "r");
    if (!file) {
        return false;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("Buttons config JSON parse error");
        return false;
    }
    
    buttons.defaultPriority = doc["defaultPriority"] | buttons.defaultPriority;
    buttons.hapticFeedback = doc["hapticFeedback"] | buttons.hapticFeedback;
    buttons.pressDelay = doc["pressDelay"] | buttons.pressDelay;
    
    JsonArray buttonArray = doc["buttons"];
    for (int i = 0; i < 6 && i < buttonArray.size(); i++) {
        JsonObject btn = buttonArray[i];
        buttons.buttons[i].label = btn["label"] | buttons.buttons[i].label;
        buttons.buttons[i].instanceID = btn["instanceID"] | buttons.buttons[i].instanceID;
        buttons.buttons[i].enabled = btn["enabled"] | buttons.buttons[i].enabled;
        buttons.buttons[i].color = btn["color"] | buttons.buttons[i].color;
        buttons.buttons[i].textColor = btn["textColor"] | buttons.buttons[i].textColor;
        buttons.buttons[i].priority = btn["priority"] | buttons.buttons[i].priority;
        buttons.buttons[i].function = btn["function"] | buttons.buttons[i].function;
        buttons.buttons[i].customAction = btn["customAction"] | buttons.buttons[i].customAction;
    }
    
    LOG_INFO("Buttons config loaded");
    return true;
}

bool ConfigManager::saveButtonsConfig() {
    DynamicJsonDocument doc(2048);
    
    doc["defaultPriority"] = buttons.defaultPriority;
    doc["hapticFeedback"] = buttons.hapticFeedback;
    doc["pressDelay"] = buttons.pressDelay;
    
    JsonArray buttonArray = doc.createNestedArray("buttons");
    for (int i = 0; i < 6; i++) {
        JsonObject btn = buttonArray.createNestedObject();
        btn["label"] = buttons.buttons[i].label;
        btn["instanceID"] = buttons.buttons[i].instanceID;
        btn["enabled"] = buttons.buttons[i].enabled;
        btn["color"] = buttons.buttons[i].color;
        btn["textColor"] = buttons.buttons[i].textColor;
        btn["priority"] = buttons.buttons[i].priority;
        btn["function"] = buttons.buttons[i].function;
        btn["customAction"] = buttons.buttons[i].customAction;
    }
    
    File file = LittleFS.open(CONFIG_BUTTONS_PATH, "w");
    if (!file) {
        LOG_ERROR("Could not create buttons config file");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    LOG_INFO("Buttons config saved");
    
    if (configChangedCallback) {
        configChangedCallback("buttons");
    }
    
    return true;
}

bool ConfigManager::loadCSMAConfig() {
    if (!LittleFS.exists(CONFIG_CSMA_PATH)) {
        return false;
    }
    
    File file = LittleFS.open(CONFIG_CSMA_PATH, "r");
    if (!file) {
        return false;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("CSMA config JSON parse error");
        return false;
    }
    
    csma.busIdleTime = doc["busIdleTime"] | csma.busIdleTime;
    csma.collisionDetectTime = doc["collisionDetectTime"] | csma.collisionDetectTime;
    csma.maxTransmissionAttempts = doc["maxTransmissionAttempts"] | csma.maxTransmissionAttempts;
    csma.sendQueueSize = doc["sendQueueSize"] | csma.sendQueueSize;
    csma.maxRetriesPerTelegram = doc["maxRetriesPerTelegram"] | csma.maxRetriesPerTelegram;
    csma.minBackoffTime = doc["minBackoffTime"] | csma.minBackoffTime;
    csma.maxBackoffTime = doc["maxBackoffTime"] | csma.maxBackoffTime;
    csma.backoffMultiplier = doc["backoffMultiplier"] | csma.backoffMultiplier;
    csma.statisticsEnabled = doc["statisticsEnabled"] | csma.statisticsEnabled;
    csma.statisticsInterval = doc["statisticsInterval"] | csma.statisticsInterval;
    
    LOG_INFO("CSMA config loaded");
    return true;
}

bool ConfigManager::saveCSMAConfig() {
    DynamicJsonDocument doc(1024);
    
    doc["busIdleTime"] = csma.busIdleTime;
    doc["collisionDetectTime"] = csma.collisionDetectTime;
    doc["maxTransmissionAttempts"] = csma.maxTransmissionAttempts;
    doc["sendQueueSize"] = csma.sendQueueSize;
    doc["maxRetriesPerTelegram"] = csma.maxRetriesPerTelegram;
    doc["minBackoffTime"] = csma.minBackoffTime;
    doc["maxBackoffTime"] = csma.maxBackoffTime;
    doc["backoffMultiplier"] = csma.backoffMultiplier;
    doc["statisticsEnabled"] = csma.statisticsEnabled;
    doc["statisticsInterval"] = csma.statisticsInterval;
    
    File file = LittleFS.open(CONFIG_CSMA_PATH, "w");
    if (!file) {
        LOG_ERROR("Could not create CSMA config file");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    LOG_INFO("CSMA config saved");
    
    if (configChangedCallback) {
        configChangedCallback("csma");
    }
    
    return true;
}

// Standard-Konfigurationen setzen
void ConfigManager::setDefaultDeviceConfig() {
    device.deviceID = "5999";
    device.orientation = 3; // Landscape
    device.firmwareVersion = "2.0";
    device.hardwareVersion = "ESP32-CYD-RS485";
    device.buildDate = __DATE__ " " __TIME__;
    device.uptime = 0;
    device.debugMode = false;
}

void ConfigManager::setDefaultNetworkConfig() {
    network.apSSID = "ESP32-TouchPanel";
    network.apPassword = "service123";
    network.apEnabled = true;
    network.staSSID = "";
    network.staPassword = "";
    network.staEnabled = false;
    network.hostname = "esp32-touch";
    network.dhcpEnabled = true;
    network.webServerPort = 80;
    network.webServerEnabled = true;
}

void ConfigManager::setDefaultDisplayConfig() {
    display.brightness = 100;
    display.statusInterval = 23000;
    display.touchInvertX = true;
    display.touchInvertY = true;
    display.touchMinX = 400;
    display.touchMaxX = 3900;
    display.touchMinY = 400;
    display.touchMaxY = 3900;
    display.screenTimeout = 0; // Kein Timeout
    display.screensaverEnabled = false;
    display.theme = "dark";
}

void ConfigManager::setDefaultButtonsConfig() {
    buttons.defaultPriority = 5;
    buttons.hapticFeedback = false;
    buttons.pressDelay = 50;
    
    // 6 Standard-Buttons
    const char* defaultLabels[] = {"Taster 1", "Taster 2", "Taster 3", "Taster 4", "Taster 5", "Taster 6"};
    
    for (int i = 0; i < 6; i++) {
        buttons.buttons[i].label = defaultLabels[i];
        buttons.buttons[i].instanceID = String(17 + i);
        buttons.buttons[i].enabled = true;
        buttons.buttons[i].color = "#808080";
        buttons.buttons[i].textColor = "#FFFFFF";
        buttons.buttons[i].priority = 5;
        buttons.buttons[i].function = "BTN";
        buttons.buttons[i].customAction = "";
    }
}

void ConfigManager::setDefaultCSMAConfig() {
    csma.busIdleTime = 10;
    csma.collisionDetectTime = 5;
    csma.maxTransmissionAttempts = 3;
    csma.sendQueueSize = 10;
    csma.maxRetriesPerTelegram = 5;
    csma.minBackoffTime = 5;
    csma.maxBackoffTime = 100;
    csma.backoffMultiplier = 10;
    csma.statisticsEnabled = true;
    csma.statisticsInterval = 30000;
}

bool ConfigManager::createDefaultConfig() {
    bool success = true;
    
    success &= saveDeviceConfig();
    success &= saveNetworkConfig();
    success &= saveDisplayConfig();
    success &= saveButtonsConfig();
    success &= saveCSMAConfig();
    
    return success;
}

// =====================================
// Backup/Restore Funktionen
// =====================================

bool ConfigManager::createBackup(String backupName) {
    if (backupName.isEmpty()) {
        backupName = generateBackupFilename();
    }
    
    String backupPath = CONFIG_BACKUP_PATH + backupName + ".json";
    
    DynamicJsonDocument doc(8192);
    
    // Alle Konfigurationen in ein JSON-Dokument zusammenfassen
    JsonObject deviceObj = doc.createNestedObject("device");
    deviceObj["deviceID"] = device.deviceID;
    deviceObj["orientation"] = device.orientation;
    deviceObj["firmwareVersion"] = device.firmwareVersion;
    deviceObj["hardwareVersion"] = device.hardwareVersion;
    deviceObj["buildDate"] = device.buildDate;
    deviceObj["debugMode"] = device.debugMode;
    
    JsonObject networkObj = doc.createNestedObject("network");
    networkObj["apSSID"] = network.apSSID;
    networkObj["apPassword"] = network.apPassword;
    networkObj["apEnabled"] = network.apEnabled;
    networkObj["staSSID"] = network.staSSID;
    networkObj["staPassword"] = network.staPassword;
    networkObj["staEnabled"] = network.staEnabled;
    networkObj["hostname"] = network.hostname;
    networkObj["dhcpEnabled"] = network.dhcpEnabled;
    networkObj["webServerPort"] = network.webServerPort;
    networkObj["webServerEnabled"] = network.webServerEnabled;
    
    JsonObject displayObj = doc.createNestedObject("display");
    displayObj["brightness"] = display.brightness;
    displayObj["statusInterval"] = display.statusInterval;
    displayObj["touchInvertX"] = display.touchInvertX;
    displayObj["touchInvertY"] = display.touchInvertY;
    displayObj["touchMinX"] = display.touchMinX;
    displayObj["touchMaxX"] = display.touchMaxX;
    displayObj["touchMinY"] = display.touchMinY;
    displayObj["touchMaxY"] = display.touchMaxY;
    displayObj["screenTimeout"] = display.screenTimeout;
    displayObj["screensaverEnabled"] = display.screensaverEnabled;
    displayObj["theme"] = display.theme;
    
    JsonObject buttonsObj = doc.createNestedObject("buttons");
    buttonsObj["defaultPriority"] = buttons.defaultPriority;
    buttonsObj["hapticFeedback"] = buttons.hapticFeedback;
    buttonsObj["pressDelay"] = buttons.pressDelay;
    
    JsonArray buttonArray = buttonsObj.createNestedArray("buttons");
    for (int i = 0; i < 6; i++) {
        JsonObject btn = buttonArray.createNestedObject();
        btn["label"] = buttons.buttons[i].label;
        btn["instanceID"] = buttons.buttons[i].instanceID;
        btn["enabled"] = buttons.buttons[i].enabled;
        btn["color"] = buttons.buttons[i].color;
        btn["textColor"] = buttons.buttons[i].textColor;
        btn["priority"] = buttons.buttons[i].priority;
        btn["function"] = buttons.buttons[i].function;
        btn["customAction"] = buttons.buttons[i].customAction;
    }
    
    JsonObject csmaObj = doc.createNestedObject("csma");
    csmaObj["busIdleTime"] = csma.busIdleTime;
    csmaObj["collisionDetectTime"] = csma.collisionDetectTime;
    csmaObj["maxTransmissionAttempts"] = csma.maxTransmissionAttempts;
    csmaObj["sendQueueSize"] = csma.sendQueueSize;
    csmaObj["maxRetriesPerTelegram"] = csma.maxRetriesPerTelegram;
    csmaObj["minBackoffTime"] = csma.minBackoffTime;
    csmaObj["maxBackoffTime"] = csma.maxBackoffTime;
    csmaObj["backoffMultiplier"] = csma.backoffMultiplier;
    csmaObj["statisticsEnabled"] = csma.statisticsEnabled;
    csmaObj["statisticsInterval"] = csma.statisticsInterval;
    
    // Backup-Metadaten hinzufügen
    JsonObject metaObj = doc.createNestedObject("metadata");
    metaObj["backupName"] = backupName;
    metaObj["timestamp"] = millis();
    metaObj["version"] = "2.0";
    
    File file = LittleFS.open(backupPath, "w");
    if (!file) {
        LOG_ERROR("Could not create backup file: " + backupPath);
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    LOG_SYSTEM("Backup created", backupName);
    return true;
}

bool ConfigManager::restoreBackup(String backupName) {
    String backupPath = CONFIG_BACKUP_PATH + backupName + ".json";
    
    if (!LittleFS.exists(backupPath)) {
        LOG_ERROR("Backup file not found: " + backupPath);
        return false;
    }
    
    File file = LittleFS.open(backupPath, "r");
    if (!file) {
        LOG_ERROR("Could not open backup file: " + backupPath);
        return false;
    }
    
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_ERROR("Backup file JSON parse error: " + String(error.c_str()));
        return false;
    }
    
    // Konfigurationen aus Backup wiederherstellen
    if (doc.containsKey("device")) {
        JsonObject deviceObj = doc["device"];
        device.deviceID = deviceObj["deviceID"] | device.deviceID;
        device.orientation = deviceObj["orientation"] | device.orientation;
        device.debugMode = deviceObj["debugMode"] | device.debugMode;
    }
    
    if (doc.containsKey("network")) {
        JsonObject networkObj = doc["network"];
        network.apSSID = networkObj["apSSID"] | network.apSSID;
        network.apPassword = networkObj["apPassword"] | network.apPassword;
        network.apEnabled = networkObj["apEnabled"] | network.apEnabled;
        network.staSSID = networkObj["staSSID"] | network.staSSID;
        network.staPassword = networkObj["staPassword"] | network.staPassword;
        network.staEnabled = networkObj["staEnabled"] | network.staEnabled;
        network.hostname = networkObj["hostname"] | network.hostname;
        network.dhcpEnabled = networkObj["dhcpEnabled"] | network.dhcpEnabled;
        network.webServerPort = networkObj["webServerPort"] | network.webServerPort;
        network.webServerEnabled = networkObj["webServerEnabled"] | network.webServerEnabled;
    }
    
    if (doc.containsKey("display")) {
        JsonObject displayObj = doc["display"];
        display.brightness = displayObj["brightness"] | display.brightness;
        display.statusInterval = displayObj["statusInterval"] | display.statusInterval;
        display.touchInvertX = displayObj["touchInvertX"] | display.touchInvertX;
        display.touchInvertY = displayObj["touchInvertY"] | display.touchInvertY;
        display.touchMinX = displayObj["touchMinX"] | display.touchMinX;
        display.touchMaxX = displayObj["touchMaxX"] | display.touchMaxX;
        display.touchMinY = displayObj["touchMinY"] | display.touchMinY;
        display.touchMaxY = displayObj["touchMaxY"] | display.touchMaxY;
        display.screenTimeout = displayObj["screenTimeout"] | display.screenTimeout;
        display.screensaverEnabled = displayObj["screensaverEnabled"] | display.screensaverEnabled;
        display.theme = displayObj["theme"] | display.theme;
    }
    
    if (doc.containsKey("buttons")) {
        JsonObject buttonsObj = doc["buttons"];
        buttons.defaultPriority = buttonsObj["defaultPriority"] | buttons.defaultPriority;
        buttons.hapticFeedback = buttonsObj["hapticFeedback"] | buttons.hapticFeedback;
        buttons.pressDelay = buttonsObj["pressDelay"] | buttons.pressDelay;
        
        if (buttonsObj.containsKey("buttons")) {
            JsonArray buttonArray = buttonsObj["buttons"];
            for (int i = 0; i < 6 && i < buttonArray.size(); i++) {
                JsonObject btn = buttonArray[i];
                buttons.buttons[i].label = btn["label"] | buttons.buttons[i].label;
                buttons.buttons[i].instanceID = btn["instanceID"] | buttons.buttons[i].instanceID;
                buttons.buttons[i].enabled = btn["enabled"] | buttons.buttons[i].enabled;
                buttons.buttons[i].color = btn["color"] | buttons.buttons[i].color;
                buttons.buttons[i].textColor = btn["textColor"] | buttons.buttons[i].textColor;
                buttons.buttons[i].priority = btn["priority"] | buttons.buttons[i].priority;
                buttons.buttons[i].function = btn["function"] | buttons.buttons[i].function;
                buttons.buttons[i].customAction = btn["customAction"] | buttons.buttons[i].customAction;
            }
        }
    }
    
    if (doc.containsKey("csma")) {
        JsonObject csmaObj = doc["csma"];
        csma.busIdleTime = csmaObj["busIdleTime"] | csma.busIdleTime;
        csma.collisionDetectTime = csmaObj["collisionDetectTime"] | csma.collisionDetectTime;
        csma.maxTransmissionAttempts = csmaObj["maxTransmissionAttempts"] | csma.maxTransmissionAttempts;
        csma.sendQueueSize = csmaObj["sendQueueSize"] | csma.sendQueueSize;
        csma.maxRetriesPerTelegram = csmaObj["maxRetriesPerTelegram"] | csma.maxRetriesPerTelegram;
        csma.minBackoffTime = csmaObj["minBackoffTime"] | csma.minBackoffTime;
        csma.maxBackoffTime = csmaObj["maxBackoffTime"] | csma.maxBackoffTime;
        csma.backoffMultiplier = csmaObj["backoffMultiplier"] | csma.backoffMultiplier;
        csma.statisticsEnabled = csmaObj["statisticsEnabled"] | csma.statisticsEnabled;
        csma.statisticsInterval = csmaObj["statisticsInterval"] | csma.statisticsInterval;
    }
    
    // Alle Konfigurationen speichern
    bool success = saveAllConfigs();
    
    if (success) {
        LOG_SYSTEM("Backup restored successfully", backupName);
    } else {
        LOG_ERROR("Failed to save restored configuration");
    }
    
    return success;
}

String ConfigManager::listBackups() {
    String result = "";
    
    File root = LittleFS.open(CONFIG_BACKUP_PATH);
    if (!root || !root.isDirectory()) {
        return result;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".json")) {
            String filename = String(file.name());
            filename = filename.substring(0, filename.lastIndexOf('.'));
            if (result.length() > 0) result += ",";
            result += filename;
        }
        file = root.openNextFile();
    }
    
    return result;
}

bool ConfigManager::deleteBackup(String backupName) {
    String backupPath = CONFIG_BACKUP_PATH + backupName + ".json";
    
    if (LittleFS.exists(backupPath)) {
        bool success = LittleFS.remove(backupPath);
        if (success) {
            LOG_SYSTEM("Backup deleted", backupName);
        }
        return success;
    }
    
    return false;
}

String ConfigManager::generateBackupFilename() {
    char timestamp[32];
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    sprintf(timestamp, "backup_%lud_%02luh_%02lum", days, hours % 24, minutes % 60);
    return String(timestamp);
}

// =====================================
// Web-Interface Support
// =====================================

String ConfigManager::getConfigAsJSON(String configType) {
    DynamicJsonDocument doc(2048);
    
    if (configType == "device") {
        doc["deviceID"] = device.deviceID;
        doc["orientation"] = device.orientation;
        doc["firmwareVersion"] = device.firmwareVersion;
        doc["hardwareVersion"] = device.hardwareVersion;
        doc["buildDate"] = device.buildDate;
        doc["uptime"] = millis() / 1000;
        doc["debugMode"] = device.debugMode;
    }
    else if (configType == "network") {
        doc["apSSID"] = network.apSSID;
        doc["apPassword"] = network.apPassword;
        doc["apEnabled"] = network.apEnabled;
        doc["staSSID"] = network.staSSID;
        doc["staPassword"] = network.staPassword;
        doc["staEnabled"] = network.staEnabled;
        doc["hostname"] = network.hostname;
        doc["dhcpEnabled"] = network.dhcpEnabled;
        doc["webServerPort"] = network.webServerPort;
        doc["webServerEnabled"] = network.webServerEnabled;
    }
    else if (configType == "display") {
        doc["brightness"] = display.brightness;
        doc["statusInterval"] = display.statusInterval;
        doc["touchInvertX"] = display.touchInvertX;
        doc["touchInvertY"] = display.touchInvertY;
        doc["touchMinX"] = display.touchMinX;
        doc["touchMaxX"] = display.touchMaxX;
        doc["touchMinY"] = display.touchMinY;
        doc["touchMaxY"] = display.touchMaxY;
        doc["screenTimeout"] = display.screenTimeout;
        doc["screensaverEnabled"] = display.screensaverEnabled;
        doc["theme"] = display.theme;
    }
    else if (configType == "buttons") {
        doc["defaultPriority"] = buttons.defaultPriority;
        doc["hapticFeedback"] = buttons.hapticFeedback;
        doc["pressDelay"] = buttons.pressDelay;
        
        JsonArray buttonArray = doc.createNestedArray("buttons");
        for (int i = 0; i < 6; i++) {
            JsonObject btn = buttonArray.createNestedObject();
            btn["label"] = buttons.buttons[i].label;
            btn["instanceID"] = buttons.buttons[i].instanceID;
            btn["enabled"] = buttons.buttons[i].enabled;
            btn["color"] = buttons.buttons[i].color;
            btn["textColor"] = buttons.buttons[i].textColor;
            btn["priority"] = buttons.buttons[i].priority;
            btn["function"] = buttons.buttons[i].function;
            btn["customAction"] = buttons.buttons[i].customAction;
        }
    }
    else if (configType == "csma") {
        doc["busIdleTime"] = csma.busIdleTime;
        doc["collisionDetectTime"] = csma.collisionDetectTime;
        doc["maxTransmissionAttempts"] = csma.maxTransmissionAttempts;
        doc["sendQueueSize"] = csma.sendQueueSize;
        doc["maxRetriesPerTelegram"] = csma.maxRetriesPerTelegram;
        doc["minBackoffTime"] = csma.minBackoffTime;
        doc["maxBackoffTime"] = csma.maxBackoffTime;
        doc["backoffMultiplier"] = csma.backoffMultiplier;
        doc["statisticsEnabled"] = csma.statisticsEnabled;
        doc["statisticsInterval"] = csma.statisticsInterval;
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String ConfigManager::getAllConfigsAsJSON() {
    DynamicJsonDocument doc(8192);
    
    // Alle Konfigurationen in ein JSON zusammenfassen
    JsonObject deviceObj = doc.createNestedObject("device");
    deviceObj["deviceID"] = device.deviceID;
    deviceObj["orientation"] = device.orientation;
    deviceObj["firmwareVersion"] = device.firmwareVersion;
    deviceObj["hardwareVersion"] = device.hardwareVersion;
    deviceObj["buildDate"] = device.buildDate;
    deviceObj["uptime"] = millis() / 1000;
    deviceObj["debugMode"] = device.debugMode;
    
    JsonObject networkObj = doc.createNestedObject("network");
    networkObj["apSSID"] = network.apSSID;
    networkObj["apPassword"] = network.apPassword;
    networkObj["apEnabled"] = network.apEnabled;
    networkObj["staSSID"] = network.staSSID;
    networkObj["staPassword"] = network.staPassword;
    networkObj["staEnabled"] = network.staEnabled;
    networkObj["hostname"] = network.hostname;
    networkObj["dhcpEnabled"] = network.dhcpEnabled;
    networkObj["webServerPort"] = network.webServerPort;
    networkObj["webServerEnabled"] = network.webServerEnabled;
    
    JsonObject displayObj = doc.createNestedObject("display");
    displayObj["brightness"] = display.brightness;
    displayObj["statusInterval"] = display.statusInterval;
    displayObj["touchInvertX"] = display.touchInvertX;
    displayObj["touchInvertY"] = display.touchInvertY;
    displayObj["touchMinX"] = display.touchMinX;
    displayObj["touchMaxX"] = display.touchMaxX;
    displayObj["touchMinY"] = display.touchMinY;
    displayObj["touchMaxY"] = display.touchMaxY;
    displayObj["screenTimeout"] = display.screenTimeout;
    displayObj["screensaverEnabled"] = display.screensaverEnabled;
    displayObj["theme"] = display.theme;
    
    JsonObject buttonsObj = doc.createNestedObject("buttons");
    buttonsObj["defaultPriority"] = buttons.defaultPriority;
    buttonsObj["hapticFeedback"] = buttons.hapticFeedback;
    buttonsObj["pressDelay"] = buttons.pressDelay;
    
    JsonArray buttonArray = buttonsObj.createNestedArray("buttons");
    for (int i = 0; i < 6; i++) {
        JsonObject btn = buttonArray.createNestedObject();
        btn["label"] = buttons.buttons[i].label;
        btn["instanceID"] = buttons.buttons[i].instanceID;
        btn["enabled"] = buttons.buttons[i].enabled;
        btn["color"] = buttons.buttons[i].color;
        btn["textColor"] = buttons.buttons[i].textColor;
        btn["priority"] = buttons.buttons[i].priority;
        btn["function"] = buttons.buttons[i].function;
        btn["customAction"] = buttons.buttons[i].customAction;
        btn["active"] = false; // Wird zur Laufzeit gesetzt
    }
    
    JsonObject csmaObj = doc.createNestedObject("csma");
    csmaObj["busIdleTime"] = csma.busIdleTime;
    csmaObj["collisionDetectTime"] = csma.collisionDetectTime;
    csmaObj["maxTransmissionAttempts"] = csma.maxTransmissionAttempts;
    csmaObj["sendQueueSize"] = csma.sendQueueSize;
    csmaObj["maxRetriesPerTelegram"] = csma.maxRetriesPerTelegram;
    csmaObj["minBackoffTime"] = csma.minBackoffTime;
    csmaObj["maxBackoffTime"] = csma.maxBackoffTime;
    csmaObj["backoffMultiplier"] = csma.backoffMultiplier;
    csmaObj["statisticsEnabled"] = csma.statisticsEnabled;
    csmaObj["statisticsInterval"] = csma.statisticsInterval;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// =====================================
// File Management
// =====================================

String ConfigManager::listFiles(String directory) {
    DynamicJsonDocument doc(4096);
    JsonArray filesArray = doc.createNestedArray("files");
    
    File root = LittleFS.open(directory);
    if (!root || !root.isDirectory()) {
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    File file = root.openNextFile();
    while (file) {
        JsonObject fileObj = filesArray.createNestedObject();
        fileObj["name"] = String(file.name());
        fileObj["path"] = directory + "/" + String(file.name());
        fileObj["size"] = file.size();
        fileObj["isDirectory"] = file.isDirectory();
        
        file = root.openNextFile();
    }
    
    // Filesystem-Statistiken hinzufügen
    JsonObject statsObj = doc.createNestedObject("stats");
    ConfigManager::FilesystemStats stats = getFilesystemStats();
    statsObj["totalBytes"] = stats.totalBytes;
    statsObj["usedBytes"] = stats.usedBytes;
    statsObj["freeBytes"] = stats.freeBytes;
    statsObj["usagePercent"] = stats.usagePercent;
    statsObj["fileCount"] = stats.fileCount;
    
    String output;
    serializeJson(doc, output);
    return output;
}

ConfigManager::FilesystemStats ConfigManager::getFilesystemStats() {
    FilesystemStats stats;
    
    stats.totalBytes = LittleFS.totalBytes();
    stats.usedBytes = LittleFS.usedBytes();
    stats.freeBytes = stats.totalBytes - stats.usedBytes;
    stats.usagePercent = (float)stats.usedBytes / stats.totalBytes * 100.0;
    
    // Datei-Anzahl zählen
    stats.fileCount = 0;
    File root = LittleFS.open("/");
    if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                stats.fileCount++;
            }
            file = root.openNextFile();
        }
    }
    
    return stats;
}

bool ConfigManager::factoryReset() {
    LOG_SYSTEM("Factory reset initiated", "");
    
    // Alle Konfigurationsdateien löschen
    LittleFS.remove(CONFIG_DEVICE_PATH);
    LittleFS.remove(CONFIG_NETWORK_PATH);
    LittleFS.remove(CONFIG_DISPLAY_PATH);
    LittleFS.remove(CONFIG_BUTTONS_PATH);
    LittleFS.remove(CONFIG_CSMA_PATH);
    
    // Logs löschen
    LittleFS.remove(LOG_SYSTEM_PATH);
    LittleFS.remove(LOG_COMM_PATH);
    LittleFS.remove(LOG_ERROR_PATH);
    
    // Standard-Konfiguration setzen
    setDefaultDeviceConfig();
    setDefaultNetworkConfig();
    setDefaultDisplayConfig();
    setDefaultButtonsConfig();
    setDefaultCSMAConfig();
    
    // Standard-Konfigurationsdateien erstellen
    bool success = createDefaultConfig();
    
    if (success) {
        LOG_SYSTEM("Factory reset completed successfully", "");
    } else {
        LOG_ERROR("Factory reset failed");
    }
    
    return success;
}

void ConfigManager::setConfigChangedCallback(ConfigChangedCallback callback) {
    configChangedCallback = callback;
}

// =====================================
// Hilfsfunktionen
// =====================================

bool setupFileSystem() {
    return configManager.begin();
}

String formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    else if (bytes < 1024 * 1024 * 1024) return String(bytes / 1024.0 / 1024.0, 1) + " MB";
    else return String(bytes / 1024.0 / 1024.0 / 1024.0, 1) + " GB";
}

String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".svg")) return "image/svg+xml";
    else if (filename.endsWith(".txt")) return "text/plain";
    else if (filename.endsWith(".zip")) return "application/zip";
    else return "application/octet-stream";
}

bool isValidJSON(String jsonString) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString);
    return error == DeserializationError::Ok;
}