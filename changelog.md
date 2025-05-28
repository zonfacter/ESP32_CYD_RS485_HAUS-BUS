# CHANGELOG

Alle wichtigen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
und dieses Projekt folgt der [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [v2.5.0] - 2025-01-XX - **MAJOR RELEASE**

### 🆕 **Hinzugefügt**

#### **Touch-Modi System**
- **6 Touch-Modi** für verschiedene Anwendungszwecke (`TOUCH_MODE 0-5`)
- **Case-Switch basierte Konfiguration** - nur ein Define ändern
- **Wisch-Erkennung** verhindert versehentliche Button-Aktivierung beim Scrollen
- **Auto-Reset System** für "hängende" grüne Buttons nach Timeout
- **Debug-Ausgaben** zeigen aktiven Touch-Modus und Events
- **Bedingte Kompilierung** - minimaler Overhead bei deaktivierten Features

#### **Converter Web Service**
- **Persistente Button-Konfiguration** - Überlebt ESP32-Resets
- **Template-System** mit 6 vorgefertigten Konfigurationen
- **EEPROM-basierte Speicherung** mit Checksummen-Validierung
- **JSON-API** für externe Konfigurationstools
- **Web-Interface Integration** für Live-Konfiguration über Browser

#### **Erweiterte API-Endpunkte**
- `/api/converter/buttons` - Button-Konfiguration laden/speichern
- `/api/converter/templates` - Template-System verwalten
- `/api/converter/system` - System-Konfiguration
- `/api/converter/factory-reset` - Zurücksetzen auf Werkseinstellungen

### 🔧 **Geändert**

#### **Touch-System Verbesserungen**
- **Service-Touch Priorität** - Service-Icon hat Vorrang vor Buttons
- **Touch-Bereich Optimierung** - Überlappungs-Probleme gelöst
- **Erweiterte Touch-Diagnostics** mit detaillierten Debug-Ausgaben

#### **Code-Struktur Verbesserungen**
- **Modularer Aufbau** - Converter Web Service als separate Komponente
- **Forward-Deklarationen** für saubere Include-Struktur
- **Zirkuläre Dependencies** aufgelöst zwischen config.h und anderen Modulen

### 🐛 **Behoben**
- **Service-Icon Touch** funktioniert wieder zuverlässig
- **Button-Überlappung** mit Service-Touch-Bereich eliminiert
- **"Hängende" grüne Buttons** nach Wisch-Gesten behoben
- **EEPROM-Initialisierung** für ESP32 hinzugefügt (`EEPROM.begin(512)`)
- **Include-Reihenfolge** für saubere Kompilierung korrigiert

---

## [v2.4.0] - 2025-05-22 - **LED-System & Backlight Fix**

### 🔧 **Behoben**

#### **LED-Button-Zuordnung korrigiert**
- **Problem**: LED-IDs 17-22 wurden erwartet, aber Protokoll nutzt LED-IDs 49-54
- **Lösung**: 
  - `communication.cpp`: LED-Verarbeitung von `17-22` auf `49-54` geändert
  - `menu.cpp`: Button instanceIDs von `"1-6"` auf `"17-22"` geändert
  - Debug-Ausgaben für korrekte Zuordnung erweitert

#### **LED-Helligkeitssteuerung korrigiert**
- **Problem**: `LED.49.ON.0` setzte Button trotzdem auf aktiv (grün)
- **Lösung**: Bei Helligkeit = 0 wird Button auf grau/inaktiv gesetzt
- **Verhalten**: 
  - `ON.100` → Weiß hell (aktiv)
  - `ON.0` → Grau (inaktiv)
  - `OFF` → Grau (inaktiv)

#### **Hintergrundbeleuchtung repariert**
- **Problem**: TFT_eSPI überschrieb PWM auf Pin 27 mit digitalWrite()
- **Lösung**:
  - `User_Setup.h`: TFT_BL Definitionen deaktiviert
  - `backlight.cpp`: PWM-Steuerung vereinfacht und stabilisiert

### 📊 **Button-LED-Zuordnungstabelle**
| Button | BTN-ID | LED-ID | Beschreibung |
|--------|--------|--------|--------------|
| Button 1 | BTN.17 | LED.49 | Taster 1 ↔ LED 1 |
| Button 2 | BTN.18 | LED.50 | Taster 2 ↔ LED 2 |
| Button 3 | BTN.19 | LED.51 | Taster 3 ↔ LED 3 |
| Button 4 | BTN.20 | LED.52 | Taster 4 ↔ LED 4 |
| Button 5 | BTN.21 | LED.53 | Taster 5 ↔ LED 5 |
| Button 6 | BTN.22 | LED.54 | Taster 6 ↔ LED 6 |

### 🧪 **Test-Befehle**
```
ý5999.LED.49.ON.100þ    # Button 1 → Weiß hell
ý5999.LED.49.ON.0þ      # Button 1 → Grau (aus)
ý5999.LBN.16.SET_MBR.50þ # Backlight → 50%
```

---

## [v2.3.0] - 2025-05-15 - **Service-Manager & Header-Display**

### 🆕 **Hinzugefügt**
- **Service-Manager** mit Touch-Aktivierung über Service-Icon
- **Header-Display** mit Zeit, Datum, Device-ID und Service-Icon
- **Device-ID Konfiguration** über Service-Menü
- **Orientierungs-Umschaltung** zwischen Portrait/Landscape
- **WiFi Access Point** für Service-Konfiguration
- **EEPROM-basierte Konfigurationsspeicherung**

### 🔧 **Geändert**
- **Touch-Interface** erweitert um Service-Icon-Bereich
- **Button-Layout** dynamisch basierend auf Orientierung
- **Display-Initialisierung** mit korrekter Orientierung

---

## [v2.2.0] - 2025-05-10 - **CSMA/CD Implementation**

### 🆕 **Hinzugefügt**
- **CSMA/CD Protokoll** für RS485-Kommunikation
- **Carrier Sense** - Lauschen vor dem Senden
- **Collision Detection** - Erkennung von Kollisionen
- **Prioritätsbasierter Sendepuffer** mit automatischer Wiederholung
- **Exponentieller Backoff-Algorithmus** bei Kollisionen
- **Kommunikations-Statistiken** und Monitoring

### 🔧 **Geändert**
- **RS485-Kommunikation** von einfachem Serial auf CSMA/CD umgestellt
- **Separate UART2-Instanz** für RS485 (RX=21, TX=22)
- **Telegramm-Verarbeitung** mit Kollisionsvermeidung

---

## [v2.1.0] - 2025-05-05 - **Touch-Interface Verbesserungen**

### 🆕 **Hinzugefügt**
- **6-Button Touch-Interface** mit dynamischem Layout
- **Portrait/Landscape Unterstützung** mit automatischer Anpassung
- **Touch-Kalibrierung** für verschiedene Display-Orientierungen
- **Button-Status Visualisierung** (grün=aktiv, grau=inaktiv)

### 🔧 **Geändert**
- **Button-Positionierung** dynamisch basierend auf Display-Größe
- **Touch-Koordinaten Mapping** für verschiedene Rotationen

---

## [v2.0.0] - 2025-05-01 - **Komplett-Rewrite**

### 🆕 **Hinzugefügt**
- **TFT-Display Support** (ST7789 Controller)
- **Resistiver Touchscreen** (XPT2046 Controller)
- **RS485-Kommunikation** über UART2
- **RGB-LED Anzeigen** für Sende-/Empfangsstatus
- **PWM-Hintergrundbeleuchtung** mit Helligkeitssteuerung

### 🔧 **Geändert**
- **Komplette Neuentwicklung** basierend auf ESP32
- **Hardware-Plattform** gewechselt zu ESP32-2432S028 (Cheap Yellow Display)
- **Kommunikationsprotokoll** erweitert für Hausbus-System

---

## [v1.0.0] - 2025-04-01 - **Erste Release**

### 🆕 **Hinzugefügt**
- **Grundlegende RS485-Kommunikation**
- **Simple Button-Interface**
- **Device-ID Konfiguration**
- **Basis-Telegramm-Verarbeitung**

---

## 🎯 **Touch-Modi Übersicht** (ab v2.5.0)

```cpp
TOUCH_MODE 0: LEGACY_MODE      - Originaler Code ohne Änderungen
TOUCH_MODE 1: NORMAL_MODE      - Wisch-Schutz + Auto-Reset (empfohlen)
TOUCH_MODE 2: AUTO_RESET_ONLY  - Nur Auto-Reset, kein Wisch-Schutz
TOUCH_MODE 3: SWIPE_ONLY       - Nur Wisch-Erkennung
TOUCH_MODE 4: SWIPE_APP_MODE   - Für Wisch-basierte Anwendungen
TOUCH_MODE 5: SENSITIVE_MODE   - Sehr empfindliche Erkennung
```

## 🏗️ **Template-System** (ab v2.5.0)

- **Wohnzimmer**: Deckenlampe, Stehlampe, TV, Rollade, Heizung, Lüftung
- **Küche**: Arbeitsplatte, Spüle, Herd, Dunstabzug, Fenster, Radio
- **Schlafzimmer**: Hauptlicht, Nachttisch, Rollade, Heizung, Lüftung, Alarm
- **Büro**: Schreibtisch, Monitor, Drucker, Jalousie, Klima, Musik
- **Bad**: Spiegel, Dusche, Lüftung, Heizung, Fenster, Nachtlicht
- **Rolladen**: Wohnen Auf/Ab, Küche Auf/Ab, Alle Auf/Ab

## 📱 **Hardware-Konfiguration**

- **Board**: TZT ESP32 2.4" LCD (ESP32-2432S028)
- **Display**: ST7789 240x320 TFT
- **Touch**: XPT2046 Resistiver Touchscreen
- **RS485**: UART2 (RX=21, TX=22) mit CSMA/CD
- **Baudrate**: 57600 bps, 8E1
- **Standard Device-ID**: 5999

## 🔧 **Migration Guidelines**

### **Von v2.4.x zu v2.5.x**
1. **`config.h`** erweitern um `#define TOUCH_MODE 1`
2. **Converter Web Service** Dateien hinzufügen
3. **Include-Statements** erweitern
4. **Voll rückwärtskompatibel** mit `TOUCH_MODE 0`

### **Von v2.3.x zu v2.4.x**
- **Automatisch** - LED-Zuordnung wird automatisch korrigiert
- **Backlight** funktioniert ohne weitere Änderungen

## 📊 **Projekt-Statistiken**

- **Unterstützte Touch-Modi**: 6
- **API-Endpunkte**: 15+
- **Button-Templates**: 6
- **Orientierungen**: 4 (Portrait/Landscape mit USB-Position)
- **Unterstützte Baudrates**: 57600 (Standard), konfigurierbar

---

**Vollständige Dokumentation**: [README.md](README.md)  
**Installation**: [INSTALLATION.md](INSTALLATION.md)  
**Bug-Reports**: [GitHub Issues](https://github.com/username/project/issues)  
**Hardware-Guide**: [HARDWARE.md](HARDWARE.md)
