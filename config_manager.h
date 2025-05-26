/**
 * config_manager.h - Version 2.0
 * 
 * LittleFS-basierter Konfigurationsmanager für ESP32 Touch-Panel
 * - JSON-basierte Konfiguration
 * - Separate Konfigurationsdateien
 * - Backup/Restore Funktionalität
 * - Web-Interface Integration
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Konfigurationspfade
#define CONFIG_DEVICE_PATH     "/config/device.json"
#define CONFIG_NETWORK_PATH    "/config/network.json"
#define CONFIG_DISPLAY_PATH    "/config/display.json"
#define CONFIG_BUTTONS_PATH    "/config/buttons.json"
#define CONFIG_CSMA_PATH       "/config/csma.json"
#define CONFIG_BACKUP_PATH     "/backup/"
#define LOG_SYSTEM_PATH        "/logs/system.log"
#define LOG_COMM_PATH          "/logs/communication.log"
#define LOG_ERROR_PATH         "/logs/error.log"

// Konfigurationsstrukturen
struct DeviceConfig {
    String deviceID;
    int orientation;           // 0=Portrait, 1=Landscape
    String firmwareVersion;
    String hardwareVersion;
    String buildDate;
    unsigned long uptime;
    bool debugMode;
};

struct NetworkConfig {
    String apSSID;
    String apPassword;
    bool apEnabled;
    String staSSID;
    String staPassword;
    bool staEnabled;
    String hostname;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    bool dhcpEnabled;
    int webServerPort;
    bool webServerEnabled;
};

struct DisplayConfig {
    int brightness;
    int statusInterval;
    bool touchInvertX;
    bool touchInvertY;
    int touchMinX;
    int touchMaxX;
    int touchMinY;
    int touchMaxY;
    int screenTimeout;
    bool screensaverEnabled;
    String theme;  // "light", "dark", "custom"
};

struct ButtonConfig {
    struct Button {
        String label;
        String instanceID;
        bool enabled;
        String color;       // HTML Hex-Code
        String textColor;   // HTML Hex-Code
        int priority;
        String function;    // "BTN", "LED", "SYSTEM"
        String customAction;
    };
    Button buttons[6];
    int defaultPriority;
    bool hapticFeedback;
    int pressDelay;
};

struct CSMAConfig {
    unsigned long busIdleTime;
    unsigned long collisionDetectTime;
    int maxTransmissionAttempts;
    int sendQueueSize;
    int maxRetriesPerTelegram;
    int minBackoffTime;
    int maxBackoffTime;
    int backoffMultiplier;
    bool statisticsEnabled;
    int statisticsInterval;
};

// Logger-Klasse für strukturierte Logs
class Logger {
private:
    String logPath;
    size_t maxLogSize;
    
public:
    Logger(String path, size_t maxSize = 50000);
    
    void info(String message);
    void warning(String message);
    void error(String message);
    void debug(String message);
    void communication(String direction, String telegram);
    void system(String event, String details = "");
    
    String readLogs(int lines = 100);
    void clearLogs();
    void rotateLogs();
    
private:
    void writeLog(String level, String message);
    String getTimestamp();
};

// Hauptklasse für Konfigurationsmanagement
class ConfigManager {
private:
    bool filesystemMounted;
    Logger* systemLogger;
    Logger* commLogger;
    Logger* errorLogger;
    
    // Interne Hilfsfunktionen
    bool createDefaultConfig();
    bool createDirectoryStructure();
    String generateBackupFilename();
    bool validateConfig(const JsonDocument& doc, String configType);
    
public:
    // Konfigurationsobjekte
    DeviceConfig device;
    NetworkConfig network;
    DisplayConfig display;
    ButtonConfig buttons;
    CSMAConfig csma;
    
    // Initialisierung
    ConfigManager();
    ~ConfigManager();
    
    bool begin();
    void end();
    
    // Konfiguration laden/speichern
    bool loadAllConfigs();
    bool saveAllConfigs();
    
    bool loadDeviceConfig();
    bool saveDeviceConfig();
    
    bool loadNetworkConfig();
    bool saveNetworkConfig();
    
    bool loadDisplayConfig();
    bool saveDisplayConfig();
    
    bool loadButtonsConfig();
    bool saveButtonsConfig();
    
    bool loadCSMAConfig();
    bool saveCSMAConfig();
    
    // Backup/Restore
    bool createBackup(String backupName = "");
    bool restoreBackup(String backupName);
    String listBackups();
    bool deleteBackup(String backupName);
    
    // Web-Interface Support
    String getConfigAsJSON(String configType);
    bool setConfigFromJSON(String configType, String jsonData);
    String getAllConfigsAsJSON();
    bool setAllConfigsFromJSON(String jsonData);
    
    // Factory Reset
    bool factoryReset();
    bool resetToDefaults(String configType = "all");
    
    // File Management
    String listFiles(String directory = "/");
    bool deleteFile(String filepath);
    bool renameFile(String oldPath, String newPath);
    size_t getFileSize(String filepath);
    String getFileContent(String filepath);
    bool writeFileContent(String filepath, String content);
    
    // Statistiken und Monitoring
    struct FilesystemStats {
        size_t totalBytes;
        size_t usedBytes;
        size_t freeBytes;
        float usagePercent;
        int fileCount;
    };
    
    FilesystemStats getFilesystemStats();
    String getSystemInfo();
    
    // Logger-Zugriff
    Logger* getSystemLogger() { return systemLogger; }
    Logger* getCommLogger() { return commLogger; }
    Logger* getErrorLogger() { return errorLogger; }
    
    // Ereignis-Callbacks
    typedef void (*ConfigChangedCallback)(String configType);
    void setConfigChangedCallback(ConfigChangedCallback callback);
    
private:
    ConfigChangedCallback configChangedCallback;
    
    // Standard-Konfigurationen
    void setDefaultDeviceConfig();
    void setDefaultNetworkConfig();
    void setDefaultDisplayConfig();
    void setDefaultButtonsConfig();
    void setDefaultCSMAConfig();
};

// Globale ConfigManager Instanz
extern ConfigManager configManager;

// Hilfsfunktionen
bool setupFileSystem();
String formatBytes(size_t bytes);
String getContentType(String filename);
bool isValidJSON(String jsonString);

// Makros für einfaches Logging
#define LOG_INFO(msg)    if(configManager.getSystemLogger()) configManager.getSystemLogger()->info(msg)
#define LOG_WARNING(msg) if(configManager.getSystemLogger()) configManager.getSystemLogger()->warning(msg)
#define LOG_ERROR(msg)   if(configManager.getErrorLogger()) configManager.getErrorLogger()->error(msg)
#define LOG_DEBUG(msg)   if(configManager.getSystemLogger()) configManager.getSystemLogger()->debug(msg)
#define LOG_COMM(dir, tel) if(configManager.getCommLogger()) configManager.getCommLogger()->communication(dir, tel)

// LOG_SYSTEM Makro mit optionalem zweiten Parameter
#define LOG_SYSTEM(...) _LOG_SYSTEM_IMPL(__VA_ARGS__, "")
#define _LOG_SYSTEM_IMPL(event, details, ...) if(configManager.getSystemLogger()) configManager.getSystemLogger()->system(event, details)

#endif // CONFIG_MANAGER_H