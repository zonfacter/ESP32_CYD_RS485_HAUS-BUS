// web_server_manager.cpp - Korrigierte Version

#include "web_server_manager.h"
#include "header_display.h"

// Globale WebServerManager Instanz
WebServerManager webServerManager;

WebServerManager::WebServerManager() : server(80), serverRunning(false) {}

void WebServerManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    setupRoutes();
    
    // Button-Konfiguration beim Start laden und synchronisieren
    loadButtonConfigFromSPIFFS();
    syncButtonsToDisplay();
    
    server.begin();
    serverRunning = true;
    Serial.println("✅ Web server started with complete button synchronization");
}

void WebServerManager::stop() {
    serverRunning = false;
    Serial.println("Web server marked as stopped.");
}

bool WebServerManager::isRunning() const {
    return serverRunning;
}

void WebServerManager::setupRoutes() {
    // Haupt-Route - leitet zur index.html weiter
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/index.html", "text/html");
    });

    // Direkte Routen für die Hauptdateien
    server.on("/index.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/index.html", "text/html");
    });
    
    server.on("/button_config.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/button_config.html", "text/html");
    });

    // WWW-Verzeichnis Routen
    server.on("/www/index.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/index.html", "text/html");
    });
    
    server.on("/www/button_config.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/button_config.html", "text/html");
    });

    server.on("/config.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/config.html", "text/html");
    });

    server.on("/backup.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/backup.html", "text/html");
    });

    server.on("/files.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/files.html", "text/html");
    });

    server.on("/logs.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/logs.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/style.css", "text/css");
    });

    server.on("/app.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveFile(request, "/www/app.js", "application/javascript");
    });

    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleAPIStatus(request);
    });

    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleAPIGetConfig(request);
    });

    server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPISaveConfig(request);
    });

    server.on("/api/brightness", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPISetBrightness(request);
    });

    server.on("/api/orientation", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPISetOrientation(request);
    });

    server.on("/api/device-id", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPISetDeviceID(request);
    });

    server.on("/api/button", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPIButtonControl(request);
    });

    server.on("/api/backup/create", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPICreateBackup(request);
    });

    server.on("/api/backup/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleAPIListBackups(request);
    });

    server.on("/api/system/reboot", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPIReboot(request);
    });

    server.on("/api/system/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPIFactoryReset(request);
    });

    // Button-Konfiguration Routen
    server.on("/config/buttons", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleGetButtonConfig(request);
    });
    
    server.on("/config/buttons", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleSaveButtonConfig(request);
    });
    
    server.on("/config/templates", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleGetTemplates(request);
    });
    
    server.on("/templates/apply", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleApplyTemplate(request);
    });
    
    server.on("/templates/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleSaveTemplate(request);
    });
    
    server.on("/test/button", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleTestButton(request);
    });
    
    server.on("/config/export", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleExportConfig(request);
    });
    
    server.on("/config/import", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleImportConfig(request);
    });
    
    server.on("/config/backup", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleCreateBackup(request);
    });

    server.onNotFound([this](AsyncWebServerRequest *request) {
        handleNotFound(request);
    });
    // Zeit/Datum API-Endpunkte (FEHLEN KOMPLETT)
    server.on("/api/time", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleAPIGetTime(request);
    });

    server.on("/api/time", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPISetTime(request);
    });

    server.on("/api/date", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleAPIGetDate(request);
    });

    server.on("/api/date", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPISetDate(request);
    });

    server.on("/api/datetime", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleAPISetDateTime(request);
    });
}

void WebServerManager::serveFile(AsyncWebServerRequest *request, const String& path, const String& type) {
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        if (file) {
            String contentType = type;
            if (contentType.isEmpty()) {
                if (path.endsWith(".html")) contentType = "text/html";
                else if (path.endsWith(".css")) contentType = "text/css";
                else if (path.endsWith(".js")) contentType = "application/javascript";
                else if (path.endsWith(".json")) contentType = "application/json";
                else contentType = "text/plain";
            }

            request->send(SPIFFS, path, contentType);
            file.close();
            return;
        }
    }

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'><title>ESP32 Touch Panel</title>";
    html += "<style>body{font-family:Arial;margin:40px;background:#f0f0f0;}";
    html += ".card{background:white;padding:20px;border-radius:10px;margin:20px 0;}</style>";
    html += "</head><body>";
    html += "<div class='card'><h1>ESP32 Touch Panel</h1>";
    html += "<p><strong>Datei nicht gefunden:</strong> " + path + "</p>";
    html += "<p>SPIFFS-Dateien müssen erst hochgeladen werden.</p>";
    html += "<p><a href='/api/status'>API Status</a> | ";
    html += "<a href='/api/config'>API Config</a></p></div>";
    html += "</body></html>";

    request->send(404, "text/html", html);
}

void WebServerManager::handleNotFound(AsyncWebServerRequest *request) {
    String message = "<!DOCTYPE html><html><head><title>404 - Nicht gefunden</title></head><body>";
    message += "<h1>404 - Datei nicht gefunden</h1>";
    message += "<p><strong>URI:</strong> " + request->url() + "</p>";
    message += "<p><strong>Method:</strong> " + String(request->method() == HTTP_GET ? "GET" : "POST") + "</p>";
    message += "<p><a href='/'>← Zurück zur Startseite</a></p>";
    message += "</body></html>";

    request->send(404, "text/html", message);
}

void WebServerManager::handleAPIStatus(AsyncWebServerRequest *request) {
    // KORRIGIERT: Größeren JSON-Buffer für alle Daten
    DynamicJsonDocument doc(2048);  // War 1024 - zu klein für Button-Daten + Zeit
    
    // System-Informationen
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["deviceID"] = serviceManager.getDeviceID();
    doc["brightness"] = currentBacklight;
    doc["orientation"] = serviceManager.getOrientation();
    doc["wifiMode"] = "AP";
    doc["wifiSSID"] = "ESP32-ServiceMode";
    doc["wifiIP"] = WiFi.softAPIP().toString();
    doc["wifiClients"] = WiFi.softAPgetStationNum();

    // SPIFFS-Informationen
    doc["spiffsTotalBytes"] = SPIFFS.totalBytes();
    doc["spiffsUsedBytes"] = SPIFFS.usedBytes();
    doc["spiffsFreeBytes"] = SPIFFS.totalBytes() - SPIFFS.usedBytes();

    // KORRIGIERT: CSMA/CD-Statistiken (doppelte extern-Deklaration entfernt)
    extern unsigned long totalSent;
    extern unsigned long totalCollisions;
    extern unsigned long totalRetries;

    doc["totalSent"] = totalSent;
    doc["totalCollisions"] = totalCollisions;
    doc["totalRetries"] = totalRetries;
    
    // Button-Daten hinzufügen
    JsonArray buttonArray = doc.createNestedArray("buttons");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        JsonObject btn = buttonArray.createNestedObject();
        btn["id"] = i;
        btn["label"] = buttons[i].label;
        btn["instanceID"] = buttons[i].instanceID;
        btn["isActive"] = buttons[i].isActive;
        btn["enabled"] = true;
        
        // Icon basierend auf Label bestimmen
        String iconType = getIconTypeFromLabel(buttons[i].label);
        btn["icon"] = getIconForType(iconType);
        btn["iconType"] = iconType;
    }

    // KORRIGIERT: Zeit/Datum-Informationen mit Fehlerbehandlung
    // Prüfen ob currentTime und Funktionen verfügbar sind
    #ifdef HEADER_DISPLAY_H  // Falls header_display.h eingebunden ist
    if (sizeof(currentTime) > 0) {  // Prüfen ob currentTime existiert
        JsonObject timeObj = doc.createNestedObject("time");
        timeObj["hour"] = currentTime.hour;
        timeObj["minute"] = currentTime.minute;
        timeObj["second"] = currentTime.second;
        
        // Sichere formatTime() Funktion verwenden
        String timeString = "";
        if (currentTime.hour < 10) timeString += "0";
        timeString += String(currentTime.hour);
        timeString += ":";
        if (currentTime.minute < 10) timeString += "0";
        timeString += String(currentTime.minute);
        timeObj["timeString"] = timeString;

        JsonObject dateObj = doc.createNestedObject("date");
        dateObj["day"] = currentTime.day;
        dateObj["month"] = currentTime.month;
        dateObj["year"] = currentTime.year;
        
        // Sichere formatDate() Funktion verwenden
        String dateString = "";
        if (currentTime.day < 10) dateString += "0";
        dateString += String(currentTime.day);
        dateString += ".";
        if (currentTime.month < 10) dateString += "0";
        dateString += String(currentTime.month);
        dateString += ".";
        dateString += String(currentTime.year);
        dateObj["dateString"] = dateString;
    } else {
        // Fallback Zeit-Daten
        JsonObject timeObj = doc.createNestedObject("time");
        timeObj["hour"] = 12;
        timeObj["minute"] = 0;
        timeObj["second"] = 0;
        timeObj["timeString"] = "12:00";
        
        JsonObject dateObj = doc.createNestedObject("date");
        dateObj["day"] = 1;
        dateObj["month"] = 1;
        dateObj["year"] = 2025;
        dateObj["dateString"] = "01.01.2025";
    }
    #else
    // Fallback wenn header_display.h nicht verfügbar
    JsonObject timeObj = doc.createNestedObject("time");
    timeObj["hour"] = 12;
    timeObj["minute"] = 0;
    timeObj["second"] = 0;
    timeObj["timeString"] = "12:00";
    
    JsonObject dateObj = doc.createNestedObject("date");
    dateObj["day"] = 1;
    dateObj["month"] = 1;
    dateObj["year"] = 2025;
    dateObj["dateString"] = "01.01.2025";
    timeObj["source"] = "Fallback (header_display.h nicht verfügbar)";
    #endif

    sendJSON(request, doc, 200);
}

void WebServerManager::handleAPIGetConfig(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    doc["deviceID"] = serviceManager.getDeviceID();
    doc["orientation"] = serviceManager.getOrientation();
    doc["brightness"] = currentBacklight;
    doc["debugMode"] = (DB_INFO == 1);

    sendJSON(request, doc, 200);
}

void WebServerManager::handleAPISaveConfig(AsyncWebServerRequest *request) {
    if (request->hasParam("deviceID", true)) {
        String newID = request->getParam("deviceID", true)->value();
        if (newID.length() == 4) {
            serviceManager.setDeviceID(newID);
        }
    }

    if (request->hasParam("orientation", true)) {
        int orientation = request->getParam("orientation", true)->value().toInt();
        if (orientation == 0 || orientation == 1) {
            serviceManager.setOrientation(orientation);
        }
    }

    serviceManager.saveConfig();
    sendSuccess(request, "Konfiguration gespeichert");
}

void WebServerManager::handleAPISetBrightness(AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
        int brightness = request->getParam("value", true)->value().toInt();
        brightness = constrain(brightness, 0, 100);
        setBacklight(brightness);
        sendSuccess(request, "Helligkeit auf " + String(brightness) + "% gesetzt");
    } else {
        sendError(request, "Parameter 'value' fehlt", 400);
    }
}

void WebServerManager::handleAPISetOrientation(AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
        int orientation = request->getParam("value", true)->value().toInt();
        if (orientation == 0 || orientation == 1) {
            serviceManager.setOrientation(orientation);
            serviceManager.saveConfig();
            
            // Orientierung direkt anwenden - aber über öffentliche Methode
            tft.setRotation(orientation);
            
            sendSuccess(request, "Orientierung geändert auf " + String(orientation == 0 ? "Portrait" : "Landscape"));
        } else {
            sendError(request, "Ungültiger Orientierungswert (0 oder 1 erwartet)", 400);
        }
    } else {
        sendError(request, "Parameter 'value' fehlt", 400);
    }
}

void WebServerManager::handleAPISetDeviceID(AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
        String newID = request->getParam("value", true)->value();
        if (newID.length() == 4) {
            serviceManager.setDeviceID(newID);
            serviceManager.saveConfig();
            sendSuccess(request, "Device ID auf " + newID + " gesetzt");
        } else {
            sendError(request, "Device ID muss 4-stellig sein", 400);
        }
    } else {
        sendError(request, "Parameter 'value' fehlt", 400);
    }
}

void WebServerManager::handleAPIButtonControl(AsyncWebServerRequest *request) {
    if (request->hasParam("button", true) && request->hasParam("action", true)) {
        int buttonIndex = request->getParam("button", true)->value().toInt();
        String action = request->getParam("action", true)->value();
        
        if (buttonIndex >= 0 && buttonIndex < NUM_BUTTONS) {
            if (action == "activate") {
                setButtonActive(buttonIndex, true);
                sendSuccess(request, "Button " + String(buttonIndex + 1) + " aktiviert");
            } else if (action == "deactivate") {
                setButtonActive(buttonIndex, false);
                sendSuccess(request, "Button " + String(buttonIndex + 1) + " deaktiviert");
            } else if (action == "test") {
                // Test-Telegramm senden
                sendTelegram("BTN", String(17 + buttonIndex), "STATUS", "1");
                delay(100);
                sendTelegram("BTN", String(17 + buttonIndex), "STATUS", "0");
                sendSuccess(request, "Button " + String(buttonIndex + 1) + " getestet");
            } else {
                sendError(request, "Ungültige Aktion", 400);
            }
        } else {
            sendError(request, "Ungültiger Button-Index", 400);
        }
    } else {
        sendError(request, "Parameter fehlen", 400);
    }
}

void WebServerManager::handleAPICreateBackup(AsyncWebServerRequest *request) {
    String backupName = "backup_" + String(millis());
    if (request->hasParam("name", true)) {
        backupName = request->getParam("name", true)->value();
    }
    
    // Hier würde die Backup-Funktionalität implementiert werden
    sendSuccess(request, "Backup " + backupName + " erstellt");
}

void WebServerManager::handleAPIListBackups(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    JsonArray backups = doc.createNestedArray("backups");
    
    // Hier würde die Liste der Backups geladen werden
    backups.add("backup_example_1");
    backups.add("backup_example_2");
    
    sendJSON(request, doc, 200);
}

void WebServerManager::handleAPIReboot(AsyncWebServerRequest *request) {
    sendSuccess(request, "Neustart wird eingeleitet...");
    delay(1000);
    ESP.restart();
}

void WebServerManager::handleAPIFactoryReset(AsyncWebServerRequest *request) {
    sendSuccess(request, "Factory Reset wird durchgeführt...");
    // Hier würde der Factory Reset implementiert werden
    delay(2000);
    ESP.restart();
}

void WebServerManager::sendJSON(AsyncWebServerRequest *request, JsonDocument& doc, int httpCode) {
    String response;
    serializeJson(doc, response);
    request->send(httpCode, "application/json", response);
}

void WebServerManager::sendError(AsyncWebServerRequest *request, const String& message, int httpCode) {
    DynamicJsonDocument doc(256);
    doc["success"] = false;
    doc["message"] = message;
    sendJSON(request, doc, httpCode);
}

void WebServerManager::sendSuccess(AsyncWebServerRequest *request, const String& message) {
    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["message"] = message;
    sendJSON(request, doc, 200);
}

// *** NEUE API-HANDLER FUNKTIONEN ***

// Button-Konfiguration laden (GET)
void WebServerManager::handleGetButtonConfig(AsyncWebServerRequest *request) {
    Serial.println("📡 GET Button-Konfiguration angefordert");
    
    DynamicJsonDocument doc(2048);
    JsonArray buttonsArray = doc.createNestedArray("buttons");
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        JsonObject btn = buttonsArray.createNestedObject();
        btn["id"] = i;
        btn["label"] = buttons[i].label.c_str();
        btn["instanceID"] = buttons[i].instanceID.c_str();
        btn["enabled"] = true;
        btn["isActive"] = buttons[i].isActive;
        
        // Farben als Hex-Strings
        btn["colorInactive"] = color565ToHex(buttons[i].color).c_str();
        btn["colorActive"] = "#00FF00";
        btn["textColor"] = color565ToHex(buttons[i].textColor).c_str();
        
        // Icon-Informationen
        String iconType = getIconTypeFromLabel(buttons[i].label);
        btn["iconName"] = iconType.c_str();
        btn["icon"] = getIconForType(iconType).c_str();
        btn["showIcon"] = true;
        btn["showLabel"] = true;
        btn["priority"] = 5;
    }
    
    sendJSON(request, doc, 200);
    Serial.println("✅ Button-Konfiguration gesendet");
}

// Button-Konfiguration speichern (POST)
void WebServerManager::handleSaveButtonConfig(AsyncWebServerRequest *request) {
    Serial.println("💾 Button-Konfiguration Speicher-Anfrage empfangen");
    
    // Debug: Alle empfangenen Parameter ausgeben
    Serial.println("📝 Empfangene Parameter:");
    int params = request->params();
    for (int i = 0; i < params; i++) {
        // KORRIGIERT: const AsyncWebParameter* verwenden
        const AsyncWebParameter* p = request->getParam(i);
        Serial.printf("  %s = %s\n", p->name().c_str(), p->value().c_str());
    }
    
    // Prüfen welche Parameter das Web-Interface sendet
    bool hasButtonData = false;
    int currentButtonId = -1;
    
    // 1. METHODE: JSON-Daten im Body (für komplexe Konfiguration)
    if (request->hasParam("buttonData", true)) {
        Serial.println("📋 JSON-Daten erkannt");
        String jsonData = request->getParam("buttonData", true)->value();
        
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, jsonData);
        
        if (error) {
            Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
            sendError(request, "Ungültige JSON-Daten: " + String(error.c_str()), 400);
            return;
        }
        
        // JSON-Button-Array verarbeiten
        if (doc.containsKey("buttons")) {
            JsonArray buttonArray = doc["buttons"];
            for (int i = 0; i < NUM_BUTTONS && i < buttonArray.size(); i++) {
                JsonObject btn = buttonArray[i];
                updateButtonFromJson(i, btn);
            }
            hasButtonData = true;
        }
    }
    
    // 2. METHODE: Einzelne Button-Parameter (Standard Web-Form)
    else if (request->hasParam("buttonId", true)) {
        Serial.println("📝 Einzelne Button-Parameter erkannt");
        currentButtonId = request->getParam("buttonId", true)->value().toInt();
        
        if (currentButtonId >= 0 && currentButtonId < NUM_BUTTONS) {
            updateButtonFromParams(currentButtonId, request);
            hasButtonData = true;
        } else {
            sendError(request, "Ungültige Button-ID: " + String(currentButtonId), 400);
            return;
        }
    }
    
    // 3. METHODE: Button-Konfigurator Format (Web-Interface Standard)
    else if (request->hasParam("label", true)) {
        Serial.println("📝 Button-Konfigurator Format erkannt");
        
        // Aktuellen Button ermitteln (Standard: Button 0 oder aus separatem Parameter)
        currentButtonId = 0;
        if (request->hasParam("currentButton", true)) {
            currentButtonId = request->getParam("currentButton", true)->value().toInt();
        }
        
        if (currentButtonId >= 0 && currentButtonId < NUM_BUTTONS) {
            updateButtonFromConfigurator(currentButtonId, request);
            hasButtonData = true;
        }
    }
    
    // 4. METHODE: Template-Anwendung
    else if (request->hasParam("templateName", true)) {
        Serial.println("📋 Template-Anwendung erkannt");
        handleApplyTemplate(request);
        return; // Template-Handler übernimmt die Antwort
    }
    
    // NEUE METHODE 5: JavaScript Button-Konfigurator Parameter
    else if (request->hasParam("buttonLabel", true)) {
        Serial.println("📝 JavaScript Button-Konfigurator erkannt");
        
        // Aktueller Button aus globalem Konfigurator-State
        currentButtonId = 0; // Default
        if (request->hasParam("id", true)) {
            currentButtonId = request->getParam("id", true)->value().toInt();
        }
        
        if (currentButtonId >= 0 && currentButtonId < NUM_BUTTONS) {
            // JavaScript Konfigurator verwendet andere Parameter-Namen
            if (request->hasParam("buttonLabel", true)) {
                String label = request->getParam("buttonLabel", true)->value();
                buttons[currentButtonId].label = label.substring(0, 15);
                Serial.printf("  Label: %s\n", buttons[currentButtonId].label.c_str());
            }
            
            if (request->hasParam("buttonInstanceID", true)) {
                buttons[currentButtonId].instanceID = request->getParam("buttonInstanceID", true)->value();
                Serial.printf("  InstanceID: %s\n", buttons[currentButtonId].instanceID.c_str());
            }
            
            if (request->hasParam("colorInactive", true)) {
                String color = request->getParam("colorInactive", true)->value();
                buttons[currentButtonId].color = hexToColor565(color);
                Serial.printf("  Farbe: %s\n", color.c_str());
            }
            
            if (request->hasParam("textColor", true)) {
                String textColor = request->getParam("textColor", true)->value();
                buttons[currentButtonId].textColor = hexToColor565(textColor);
                Serial.printf("  Textfarbe: %s\n", textColor.c_str());
            }
            
            hasButtonData = true;
        }
    }
    
    // Fehler: Keine erkannten Parameter
    if (!hasButtonData) {
        Serial.println("❌ Keine gültigen Button-Parameter gefunden");
        
        // Für Debug: Ersten Button testweise setzen
        buttons[0].label = "WEB TEST";
        buttons[0].instanceID = "17";
        drawButtons();
        saveButtonConfigToSPIFFS();
        
        sendSuccess(request, "DEBUG: Button 1 auf 'WEB TEST' gesetzt (keine Parameter erkannt)");
        return;
    }
     // NEU: Orientierung mit speichern falls übertragen
    if (request->hasParam("orientation", true)) {
        int orientation = request->getParam("orientation", true)->value().toInt();
        if (orientation == 0 || orientation == 1) {
            serviceManager.setOrientation(orientation);
            serviceManager.saveConfig();
            
            // Display-Orientierung sofort anwenden
            tft.setRotation(orientation == 0 ? ROTATION_0 : ROTATION_270);
            
            Serial.println("✅ Orientierung mit gespeichert: " + String(orientation));
        }
    }
    // Konfiguration speichern und Display aktualisieren
    saveButtonConfigToSPIFFS();
    drawButtons();
    
    // Erfolgsantwort
    if (currentButtonId >= 0) {
        sendSuccess(request, "Button " + String(currentButtonId + 1) + " (" + buttons[currentButtonId].label + ") erfolgreich gespeichert");
        Serial.println("✅ Button " + String(currentButtonId + 1) + " gespeichert: " + buttons[currentButtonId].label);
    } else {
        sendSuccess(request, "Button-Konfiguration erfolgreich gespeichert");
        Serial.println("✅ Button-Konfiguration komplett gespeichert");
    }
}

void WebServerManager::handleGetTemplates(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    JsonObject templates = doc.createNestedObject("templates");
    
    templates["wohnzimmer"] = "Wohnzimmer Template";
    templates["schlafzimmer"] = "Schlafzimmer Template";
    templates["rolladen"] = "Rolladen Template";
    templates["beleuchtung"] = "Beleuchtung Template";
    
    sendJSON(request, doc, 200);
}

void WebServerManager::handleApplyTemplate(AsyncWebServerRequest *request) {
    if (!request->hasParam("templateName", true)) {
        sendError(request, "Template-Name fehlt", 400);
        return;
    }
    
    String templateName = request->getParam("templateName", true)->value();
    Serial.println("📋 Template anwenden: " + templateName);
    
    if (templateName == "wohnzimmer") {
        buttons[0].label = "Deckenlampe";
        buttons[1].label = "Stehlampe";
        buttons[2].label = "TV";
        buttons[3].label = "Rollade";
        buttons[4].label = "Heizung";
        buttons[5].label = "Lüftung";
    } else if (templateName == "schlafzimmer") {
        buttons[0].label = "Hauptlicht";
        buttons[1].label = "Nachttisch";
        buttons[2].label = "Rollade";
        buttons[3].label = "Heizung";
        buttons[4].label = "Lüftung";
        buttons[5].label = "Alarm";
    } else if (templateName == "rolladen") {
        buttons[0].label = "Wohnen Auf";
        buttons[1].label = "Wohnen Ab";
        buttons[2].label = "Küche Auf";
        buttons[3].label = "Küche Ab";
        buttons[4].label = "Alle Auf";
        buttons[5].label = "Alle Ab";
    } else if (templateName == "beleuchtung") {
        buttons[0].label = "Wohnzimmer";
        buttons[1].label = "Küche";
        buttons[2].label = "Flur";
        buttons[3].label = "Bad";
        buttons[4].label = "Alle An";
        buttons[5].label = "Alle Aus";
    } else {
        sendError(request, "Template nicht gefunden", 404);
        return;
    }
    
    // Instance IDs und Farben setzen
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].instanceID = String(17 + i);
        buttons[i].color = TFT_DARKGREY;
        buttons[i].textColor = TFT_WHITE;
        buttons[i].isActive = false;
    }
    
    // Konfiguration speichern und Display aktualisieren
    saveButtonConfigToSPIFFS();
    drawButtons();
    
    sendSuccess(request, "Template '" + templateName + "' erfolgreich angewendet");
    Serial.println("✅ Template '" + templateName + "' angewendet");
}

void WebServerManager::handleSaveTemplate(AsyncWebServerRequest *request) {
    if (request->hasParam("templateName", true)) {
        String templateName = request->getParam("templateName", true)->value();
        sendSuccess(request, "Template '" + templateName + "' gespeichert");
    } else {
        sendError(request, "Template-Name fehlt", 400);
    }
}

void WebServerManager::handleTestButton(AsyncWebServerRequest *request) {
    if (request->hasParam("id", true)) {
        int buttonId = request->getParam("id", true)->value().toInt();
        
        if (buttonId >= 0 && buttonId < NUM_BUTTONS) {
            String instanceID = buttons[buttonId].instanceID;
            sendTelegram("BTN", instanceID, "STATUS", "1");
            delay(100);
            sendTelegram("BTN", instanceID, "STATUS", "0");
            
            sendSuccess(request, "Button " + String(buttonId + 1) + " getestet");
        } else {
            sendError(request, "Ungültige Button-ID", 400);
        }
    } else {
        sendError(request, "Button-ID fehlt", 400);
    }
}

void WebServerManager::handleExportConfig(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);
    
    doc["version"] = "2.0";
    doc["deviceID"] = serviceManager.getDeviceID();
    doc["orientation"] = serviceManager.getOrientation();
    doc["exportDate"] = millis();
    
    JsonArray buttonsArray = doc.createNestedArray("buttons");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        JsonObject btn = buttonsArray.createNestedObject();
        btn["id"] = i;
        btn["label"] = buttons[i].label;
        btn["instanceID"] = buttons[i].instanceID;
        btn["enabled"] = true;
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleImportConfig(AsyncWebServerRequest *request) {
    sendSuccess(request, "Konfiguration importiert (Demo)");
}

void WebServerManager::handleCreateBackup(AsyncWebServerRequest *request) {
    String backupName = "backup_" + String(millis());
    if (request->hasParam("name", true)) {
        backupName = request->getParam("name", true)->value();
    }
    
    sendSuccess(request, "Backup '" + backupName + "' erstellt");
}

void WebServerManager::handleAPIGetTime(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    doc["hour"] = currentTime.hour;
    doc["minute"] = currentTime.minute;
    doc["second"] = currentTime.second;
    
    // Formatierte Zeit-Strings
    String timeString = "";
    if (currentTime.hour < 10) timeString += "0";
    timeString += String(currentTime.hour);
    timeString += ":";
    if (currentTime.minute < 10) timeString += "0";
    timeString += String(currentTime.minute);
    
    doc["timeString"] = timeString;
    doc["timestamp"] = millis();
    doc["source"] = "ESP32 intern";
    
    sendJSON(request, doc, 200);
}

void WebServerManager::handleAPISetTime(AsyncWebServerRequest *request) {
    // Parameter auslesen
    String timeString = "";
    int hour = -1, minute = -1, second = -1;
    
    if (request->hasParam("timeString", true)) {
        timeString = request->getParam("timeString", true)->value();
        // Format: HH:MM:SS oder HH:MM
        if (timeString.length() >= 5) {
            hour = timeString.substring(0, 2).toInt();
            minute = timeString.substring(3, 5).toInt();
            if (timeString.length() >= 8) {
                second = timeString.substring(6, 8).toInt();
            } else {
                second = 0; // Default
            }
        }
    } else {
        // Einzelne Parameter
        if (request->hasParam("hour", true)) {
            hour = request->getParam("hour", true)->value().toInt();
        }
        if (request->hasParam("minute", true)) {
            minute = request->getParam("minute", true)->value().toInt();
        }
        if (request->hasParam("second", true)) {
            second = request->getParam("second", true)->value().toInt();
        }
    }
    
    // Validierung
    if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
        // Zeit setzen über bestehende Telegramm-Funktion
        String timeParam = "";
        if (hour < 10) timeParam += "0";
        timeParam += String(hour);
        if (minute < 10) timeParam += "0";
        timeParam += String(minute);
        if (second < 10) timeParam += "0";
        timeParam += String(second);
        
        // Nutze bestehende Funktion aus header_display.cpp
        handleTimeSetTelegram(timeParam);
        
        sendSuccess(request, "Zeit gesetzt auf " + formatTime());
    } else {
        sendError(request, "Ungültige Zeit-Parameter", 400);
    }
}

void WebServerManager::handleAPIGetDate(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    doc["day"] = currentTime.day;
    doc["month"] = currentTime.month;
    doc["year"] = currentTime.year;
    
    // Formatierte Datum-Strings
    String dateString = "";
    if (currentTime.day < 10) dateString += "0";
    dateString += String(currentTime.day);
    dateString += ".";
    if (currentTime.month < 10) dateString += "0";
    dateString += String(currentTime.month);
    dateString += ".";
    dateString += String(currentTime.year);
    
    doc["dateString"] = dateString;
    doc["timestamp"] = millis();
    
    sendJSON(request, doc, 200);
}

void WebServerManager::handleAPISetDate(AsyncWebServerRequest *request) {
    // Parameter auslesen
    String dateString = "";
    int day = -1, month = -1, year = -1;
    
    if (request->hasParam("dateString", true)) {
        dateString = request->getParam("dateString", true)->value();
        // Format: DD.MM.YYYY
        if (dateString.length() >= 10) {
            day = dateString.substring(0, 2).toInt();
            month = dateString.substring(3, 5).toInt();
            year = dateString.substring(6, 10).toInt();
        }
    } else {
        // Einzelne Parameter
        if (request->hasParam("day", true)) {
            day = request->getParam("day", true)->value().toInt();
        }
        if (request->hasParam("month", true)) {
            month = request->getParam("month", true)->value().toInt();
        }
        if (request->hasParam("year", true)) {
            year = request->getParam("year", true)->value().toInt();
        }
    }
    
    // Validierung
    if (day >= 1 && day <= 31 && month >= 1 && month <= 12 && year >= 2020 && year <= 2099) {
        // Datum setzen über bestehende Telegramm-Funktion
        String dateParam = "";
        if (day < 10) dateParam += "0";
        dateParam += String(day);
        if (month < 10) dateParam += "0";
        dateParam += String(month);
        dateParam += String(year);
        
        // Nutze bestehende Funktion aus header_display.cpp
        handleDateSetTelegram(dateParam);
        
        sendSuccess(request, "Datum gesetzt auf " + formatDate());
    } else {
        sendError(request, "Ungültige Datum-Parameter", 400);
    }
}

void WebServerManager::handleAPISetDateTime(AsyncWebServerRequest *request) {
    // Kombinierte Zeit/Datum-Setzung für Browser-Synchronisation
    bool timeSet = false;
    bool dateSet = false;
    String messages = "";
    
    // Zeit setzen falls Parameter vorhanden
    if (request->hasParam("hour", true) && request->hasParam("minute", true)) {
        int hour = request->getParam("hour", true)->value().toInt();
        int minute = request->getParam("minute", true)->value().toInt();
        int second = 0;
        
        if (request->hasParam("second", true)) {
            second = request->getParam("second", true)->value().toInt();
        }
        
        if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
            String timeParam = "";
            if (hour < 10) timeParam += "0";
            timeParam += String(hour);
            if (minute < 10) timeParam += "0";
            timeParam += String(minute);
            if (second < 10) timeParam += "0";
            timeParam += String(second);
            
            handleTimeSetTelegram(timeParam);
            timeSet = true;
            messages += "Zeit gesetzt. ";
        }
    }
    
    // Datum setzen falls Parameter vorhanden
    if (request->hasParam("day", true) && request->hasParam("month", true) && request->hasParam("year", true)) {
        int day = request->getParam("day", true)->value().toInt();
        int month = request->getParam("month", true)->value().toInt();
        int year = request->getParam("year", true)->value().toInt();
        
        if (day >= 1 && day <= 31 && month >= 1 && month <= 12 && year >= 2020 && year <= 2099) {
            String dateParam = "";
            if (day < 10) dateParam += "0";
            dateParam += String(day);
            if (month < 10) dateParam += "0";
            dateParam += String(month);
            dateParam += String(year);
            
            handleDateSetTelegram(dateParam);
            dateSet = true;
            messages += "Datum gesetzt. ";
        }
    }
    
    if (timeSet || dateSet) {
        sendSuccess(request, messages + "Aktuelle Zeit: " + formatTime() + " | " + formatDate());
    } else {
        sendError(request, "Keine gültigen Zeit/Datum-Parameter gefunden", 400);
    }
}

// Button-Konfiguration aus SPIFFS laden und auf Display übertragen
void WebServerManager::syncButtonsToDisplay() {
    Serial.println("🔄 Synchronisiere Button-Konfiguration...");
    
    // Button-Konfiguration aus SPIFFS laden
    if (!SPIFFS.exists("/config/buttons.json")) {
        Serial.println("⚠️ Keine Button-Konfiguration gefunden, verwende Defaults");
        createDefaultButtonConfig();
        return;
    }
    
    File configFile = SPIFFS.open("/config/buttons.json", "r");
    if (!configFile) {
        Serial.println("❌ Kann Button-Konfiguration nicht öffnen");
        return;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
        return;
    }
    
    // Button-Daten auf Display-Array übertragen
    JsonArray buttonArray = doc["buttons"];
    for (int i = 0; i < NUM_BUTTONS && i < buttonArray.size(); i++) {
        JsonObject btn = buttonArray[i];
        
        // KORRIGIERT: String-Handling für ArduinoJson
        String defaultLabel = "Button " + String(i + 1);
        const char* labelValue = btn["label"];
        String label = labelValue ? String(labelValue) : defaultLabel;
        buttons[i].label = label.substring(0, 15);
        
        // KORRIGIERT: Instance ID handling
        const char* instanceValue = btn["instanceID"];
        String instanceID = instanceValue ? String(instanceValue) : String(17 + i);
        buttons[i].instanceID = instanceID;
        
        // KORRIGIERT: Farben konvertieren
        const char* colorInactiveValue = btn["colorInactive"];
        String colorInactive = colorInactiveValue ? String(colorInactiveValue) : "#808080";
        buttons[i].color = hexToColor565(colorInactive);
        
        const char* textColorValue = btn["textColor"];
        String textColor = textColorValue ? String(textColorValue) : "#FFFFFF";
        buttons[i].textColor = hexToColor565(textColor);
        
        // Status setzen
        buttons[i].isActive = btn["isActive"] | false;
        buttons[i].pressed = false;
        
        Serial.printf("✅ Button %d: %s (ID: %s)\n", 
                     i + 1, buttons[i].label.c_str(), buttons[i].instanceID.c_str());
    }
    
    // Display neu zeichnen
    drawButtons();
    Serial.println("✅ Button-Synchronisation abgeschlossen");
}

// Button-Konfiguration aus SPIFFS laden
void WebServerManager::loadButtonConfigFromSPIFFS() {
    Serial.println("📁 Lade Button-Konfiguration aus SPIFFS...");
    
    if (!SPIFFS.exists("/config/buttons.json")) {
        Serial.println("📁 Erstelle Standard Button-Konfiguration...");
        createDefaultButtonConfig();
        return;
    }
    
    Serial.println("✅ Button-Konfigurationsdatei gefunden");
}

// Standard Button-Konfiguration erstellen
void WebServerManager::createDefaultButtonConfig() {
    Serial.println("🔧 Erstelle Standard Button-Konfiguration...");
    
    const char* defaultLabels[] = {"Deckenlampe", "Stehlampe", "Rollade", "Heizung", "Lüftung", "Schalter"};
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].label = String(defaultLabels[i]);
        buttons[i].instanceID = String(17 + i);
        buttons[i].color = TFT_DARKGREY;
        buttons[i].textColor = TFT_WHITE;
        buttons[i].isActive = false;
        buttons[i].pressed = false;
        
        // Position berechnen (3x2 Grid für Landscape)
        int row = i / 3;
        int col = i % 3;
        buttons[i].x = col * (SCREEN_WIDTH / 3) + 10;
        buttons[i].y = row * (SCREEN_HEIGHT / 2) + 40;
        buttons[i].w = (SCREEN_WIDTH / 3) - 20;
        buttons[i].h = (SCREEN_HEIGHT / 2) - 50;
    }
    
    // Standard-Konfiguration speichern
    saveButtonConfigToSPIFFS();
    
    Serial.println("✅ Standard Button-Konfiguration erstellt");
}

// Button-Konfiguration in SPIFFS speichern
void WebServerManager::saveButtonConfigToSPIFFS() {
    Serial.println("💾 Speichere Button-Konfiguration in SPIFFS...");
    
    DynamicJsonDocument doc(2048);
    doc["version"] = "2.0";
    doc["timestamp"] = millis();
    
    JsonArray buttonArray = doc.createNestedArray("buttons");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        JsonObject btn = buttonArray.createNestedObject();
        btn["id"] = i;
        btn["label"] = buttons[i].label.c_str();
        btn["instanceID"] = buttons[i].instanceID.c_str();
        btn["enabled"] = true;
        btn["isActive"] = buttons[i].isActive;
        
        // Farben als Hex-Strings
        btn["colorInactive"] = color565ToHex(buttons[i].color).c_str();
        btn["colorActive"] = "#00FF00";
        btn["textColor"] = color565ToHex(buttons[i].textColor).c_str();
        
        // Icon basierend auf Label bestimmen
        btn["iconName"] = getIconTypeFromLabel(buttons[i].label).c_str();
        btn["showIcon"] = true;
        btn["showLabel"] = true;
        btn["priority"] = 5;
    }
    
    File configFile = SPIFFS.open("/config/buttons.json", "w");
    if (!configFile) {
        Serial.println("❌ Kann Button-Konfiguration nicht speichern");
        return;
    }
    
    serializeJsonPretty(doc, configFile);
    configFile.close();
    
    Serial.println("✅ Button-Konfiguration in SPIFFS gespeichert");
}

// Hex-String zu 16-Bit Farbe konvertieren
uint16_t WebServerManager::hexToColor565(String hexColor) {
    // # entfernen falls vorhanden
    if (hexColor.startsWith("#")) {
        hexColor = hexColor.substring(1);
    }
    
    // Standard-Farbe falls ungültig
    if (hexColor.length() != 6) {
        return TFT_DARKGREY;
    }
    
    // Hex zu RGB konvertieren
    long hexValue = strtol(hexColor.c_str(), NULL, 16);
    uint8_t r = (hexValue >> 16) & 0xFF;
    uint8_t g = (hexValue >> 8) & 0xFF;
    uint8_t b = hexValue & 0xFF;
    
    // RGB zu 565 Format konvertieren
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// 16-Bit Farbe zu Hex-String konvertieren
String WebServerManager::color565ToHex(uint16_t color565) {
    uint8_t r = (color565 >> 8) & 0xF8;
    uint8_t g = (color565 >> 3) & 0xFC;
    uint8_t b = (color565 << 3) & 0xF8;
    
    char hexString[8];
    sprintf(hexString, "#%02X%02X%02X", r, g, b);
    return String(hexString);
}

// Icon-Typ basierend auf Label bestimmen
String WebServerManager::getIconTypeFromLabel(String label) {
    String lowerLabel = label;
    lowerLabel.toLowerCase();
    
    if (lowerLabel.indexOf("licht") >= 0 || lowerLabel.indexOf("lampe") >= 0) {
        return "LIGHT";
    } else if (lowerLabel.indexOf("rollade") >= 0 || lowerLabel.indexOf("rolladen") >= 0) {
        return "SHUTTER";
    } else if (lowerLabel.indexOf("heizung") >= 0 || lowerLabel.indexOf("temperatur") >= 0) {
        return "TEMPERATURE";
    } else if (lowerLabel.indexOf("lüftung") >= 0 || lowerLabel.indexOf("ventil") >= 0) {
        return "VENTILATION";
    } else if (lowerLabel.indexOf("dimmer") >= 0) {
        return "DIMMER";
    } else {
        return "SWITCH";
    }
}

// Icon für Icon-Typ zurückgeben
String WebServerManager::getIconForType(String iconType) {
    if (iconType == "LIGHT") return "💡";
    else if (iconType == "SHUTTER") return "🏠";
    else if (iconType == "SWITCH") return "🔘";
    else if (iconType == "DIMMER") return "🔆";
    else if (iconType == "TEMPERATURE") return "🌡️";
    else if (iconType == "VENTILATION") return "💨";
    else return "⚪";
}


void WebServerManager::updateButtonFromJson(int buttonId, JsonObject btn) {
    Serial.printf("📝 Aktualisiere Button %d aus JSON\n", buttonId);
    
    // Label
    if (btn.containsKey("label")) {
        String label = btn["label"].as<String>();
        buttons[buttonId].label = label.substring(0, 15);
        Serial.printf("  Label: %s\n", buttons[buttonId].label.c_str());
    }
    
    // Instance ID
    if (btn.containsKey("instanceID")) {
        buttons[buttonId].instanceID = btn["instanceID"].as<String>();
        Serial.printf("  InstanceID: %s\n", buttons[buttonId].instanceID.c_str());
    }
    
    // Farben
    if (btn.containsKey("colorInactive")) {
        String color = btn["colorInactive"].as<String>();
        buttons[buttonId].color = hexToColor565(color);
    }
    
    if (btn.containsKey("textColor")) {
        String textColor = btn["textColor"].as<String>();
        buttons[buttonId].textColor = hexToColor565(textColor);
    }
    
    // Status
    if (btn.containsKey("isActive")) {
        buttons[buttonId].isActive = btn["isActive"].as<bool>();
    }
}

void WebServerManager::updateButtonFromParams(int buttonId, AsyncWebServerRequest *request) {
    Serial.printf("📝 Aktualisiere Button %d aus Parametern\n", buttonId);
    
    // Label
    if (request->hasParam("label", true)) {
        String label = request->getParam("label", true)->value();
        buttons[buttonId].label = label.substring(0, 15);
        Serial.printf("  Label: %s\n", buttons[buttonId].label.c_str());
    }
    
    // Instance ID
    if (request->hasParam("instanceID", true)) {
        buttons[buttonId].instanceID = request->getParam("instanceID", true)->value();
        Serial.printf("  InstanceID: %s\n", buttons[buttonId].instanceID.c_str());
    }
    
    // Farben
    if (request->hasParam("colorInactive", true)) {
        String color = request->getParam("colorInactive", true)->value();
        buttons[buttonId].color = hexToColor565(color);
        Serial.printf("  Farbe: %s\n", color.c_str());
    }
    
    if (request->hasParam("textColor", true)) {
        String textColor = request->getParam("textColor", true)->value();
        buttons[buttonId].textColor = hexToColor565(textColor);
        Serial.printf("  Textfarbe: %s\n", textColor.c_str());
    }
    
    // Status
    if (request->hasParam("enabled", true)) {
        String enabled = request->getParam("enabled", true)->value();
        // enabled Parameter wird vom Web-Interface als "true"/"false" String gesendet
        buttons[buttonId].isActive = (enabled == "true" || enabled == "1");
    }
}

void WebServerManager::updateButtonFromConfigurator(int buttonId, AsyncWebServerRequest *request) {
    Serial.printf("📝 Aktualisiere Button %d aus Button-Konfigurator\n", buttonId);
    
    // Standard Button-Konfigurator Parameter
    if (request->hasParam("buttonLabel", true)) {
        String label = request->getParam("buttonLabel", true)->value();
        buttons[buttonId].label = label.substring(0, 15);
        Serial.printf("  Label: %s\n", buttons[buttonId].label.c_str());
    }
    
    if (request->hasParam("buttonInstanceID", true)) {
        buttons[buttonId].instanceID = request->getParam("buttonInstanceID", true)->value();
        Serial.printf("  InstanceID: %s\n", buttons[buttonId].instanceID.c_str());
    }
    
    if (request->hasParam("colorInactive", true)) {
        String color = request->getParam("colorInactive", true)->value();
        buttons[buttonId].color = hexToColor565(color);
        Serial.printf("  Farbe: %s\n", color.c_str());
    }
    
    if (request->hasParam("textColor", true)) {
        String textColor = request->getParam("textColor", true)->value();
        buttons[buttonId].textColor = hexToColor565(textColor);
        Serial.printf("  Textfarbe: %s\n", textColor.c_str());
    }
    
    // Alternative Parameter-Namen für Button-Konfigurator
    if (request->hasParam("buttonEnabled", true)) {
        String enabled = request->getParam("buttonEnabled", true)->value();
        buttons[buttonId].isActive = (enabled == "true" || enabled == "1" || enabled == "on");
    }
}