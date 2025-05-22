# ESP32 Touch-Menü mit TFT Display und RS485-Kommunikation

Dieses Projekt implementiert ein interaktives Touch-Menü für TZT ESP32 2.4" LCD mit ST7789 Controller und XPT2046 Touchscreen. Es bietet ein 6-Tasten-Interface, PWM-gesteuerte Hintergrundbeleuchtung, RGB-LED-Feedback und RS485-UART-Kommunikation für industrielle Anwendungen.

## 🆕 Version 1.50 - Neue Features

- **Service-Menü System** mit 20-Sekunden Touch-Aktivierung
- **Device ID Editor** mit Touch-Numpad für 4-stellige ID-Konfiguration
- **Orientierungs-Umschaltung** (Portrait ↔ Landscape) mit sofortiger Anwendung
- **WiFi Access Point** für Service-Konfiguration und Web-Interface
- **EEPROM-Konfigurationsspeicherung** für persistente Einstellungen
- **CSMA/CD-Kommunikation** für kollisionsfreie RS485-Übertragung

## 📋 Inhaltsverzeichnis

- [Hardware-Komponenten](#hardware-komponenten)
- [Pin-Belegung](#pin-belegung)
- [RS485-TTL-Umsetzer](#rs485-ttl-umsetzer)
- [Service-Menü (NEU)](#service-menü-neu)
- [Funktionen](#funktionen)
- [Konfiguration](#konfiguration)
- [Kommunikationsprotokoll](#kommunikationsprotokoll)
- [Installation](#installation)
- [Quick-Setup Guide](#quick-setup-guide)
- [Testmodus](#testmodus)
- [Debugging](#debugging)
- [Troubleshooting](#troubleshooting)

## 🛠 Hardware-Komponenten

| Komponente | Beschreibung | Hersteller/Typ |
|------------|-------------|----------------|
| **Hauptboard** | TZT ESP32 2.4" LCD mit Touchscreen | ESP32-WROOM-32D |
| **Display** | 2.4" TFT LCD 240x320 Pixel | ST7789 Controller |
| **Touchscreen** | Resistiver Touchscreen | XPT2046 Controller |
| **Prozessor** | ESP32 Dual-Core 240MHz | Espressif ESP32 |
| **RS485-Umsetzer** | TTL zu RS485 Konverter | MAX485 oder ähnlich |
| **RGB-LED** | Status-Anzeige (optional) | Common Cathode |

## 📌 Pin-Belegung

### ESP32 TZT 2.4" LCD Board

| Funktion | GPIO-Pin | Beschreibung | Hinweise |
|----------|----------|-------------|----------|
| **Display (ST7789)** | | | |
| TFT_CS | 15 | Chip Select | Display auswählen |
| TFT_DC | 2 | Data/Command | Daten/Befehl umschalten |
| TFT_RST | -1 | Reset | Nicht verwendet (Hardware-Reset) |
| TFT_MOSI | 13 | Master Out Slave In | SPI Datenleitung |
| TFT_SCLK | 14 | Serial Clock | SPI Taktleitung |
| TFT_MISO | 12 | Master In Slave Out | SPI Rückleitung |
| TFT_BL | 27 | Backlight PWM | Hintergrundbeleuchtung |
| **Touchscreen (XPT2046)** | | | |
| T_CS | 33 | Chip Select | Touch-Controller auswählen |
| T_IRQ | 36 | Interrupt Request | Touch-Erkennung |
| T_DIN | 13 | Data In | Shared with TFT_MOSI |
| T_OUT | 12 | Data Out | Shared with TFT_MISO |
| T_CLK | 14 | Clock | Shared with TFT_SCLK |
| **UART2 (RS485)** | | | |
| UART2_RX | 21 | Receive Data | ⚠️ Hardware umverdrahtet! |
| UART2_TX | 22 | Transmit Data | ⚠️ Hardware umverdrahtet! |
| **RGB-LED (Optional)** | | | |
| LED_RED | 4 | Rote LED | Negierte Logik (LOW=Ein) |
| LED_GREEN | 16 | Grüne LED | Negierte Logik (LOW=Ein) |
| LED_BLUE | 17 | Blaue LED | Negierte Logik (LOW=Ein) |

> ⚠️ **Wichtiger Hinweis**: In dieser Hardware-Konfiguration sind RX und TX vertauscht! RX ist auf Pin 21, TX auf Pin 22.

## 🔌 RS485-TTL-Umsetzer

### Empfohlene Module
- **MAX485** - Standard RS485 Transceiver
- **SP485** - Pin-kompatible Alternative
- **SN65HVD75** - 3.3V Low-Power Variante

### Anschlussschema RS485-Umsetzer

```
ESP32 TZT Board          RS485-TTL-Umsetzer          RS485 Bus
┌─────────────────┐      ┌─────────────────────┐      ┌─────────────────┐
│                 │      │                     │      │                 │
│ GPIO22 (TX) ────┼──────┤ DI (Driver Input)   │      │                 │
│                 │      │                     │      │                 │
│ GPIO21 (RX) ────┼──────┤ RO (Receiver Out)   │      │                 │
│                 │      │                     │      │                 │
│ 3.3V VCC   ────┼──────┤ VCC                 │      │                 │
│                 │      │                     │      │                 │
│ GND        ────┼──────┤ GND                 │      │                 │
│                 │      │                     │      │                 │
│                 │      │ A/+ ────────────────┼──────┤ A/+ (Data+)     │
│                 │      │                     │      │                 │
│                 │      │ B/- ────────────────┼──────┤ B/- (Data-)     │
│                 │      │                     │      │                 │
│ N/C (Auto)  ────┼──────┤ DE (Driver Enable)  │      │                 │
│                 │      │                     │      │                 │
│ N/C (Auto)  ────┼──────┤ RE (Receiver Enable)│      │                 │
└─────────────────┘      └─────────────────────┘      └─────────────────┘
```

## 🛠 Service-Menü (NEU)

### Aktivierung des Service-Modus

#### **Methode 1: 20-Sekunden Touch**
1. **Beliebige Stelle** auf dem Bildschirm 20 Sekunden lang berühren
2. **Progress-Bar** zeigt den Fortschritt (0-100%)
3. **Service-Menü** öffnet sich automatisch

#### **Methode 2: Telegramm-Steuerung**
```bash
ý5999.SYS.1.SERVICE.1þ    # Service-Modus aktivieren
ý5999.SYS.1.SERVICE.0þ    # Service-Modus deaktivieren
```

### Service-Menü Funktionen

```
┌─────────────────────────────────────┐
│            SERVICE MODUS            │
├─────────────────────────────────────┤
│ Device ID: 5999    Orient: LANDSCAPE│
│                                     │
│  [Device ID]      [→ Portrait]      │
│                                     │
│  [WiFi ON]        [Web Config]      │
│                                     │
│  [SAVE & EXIT]    [CANCEL]          │
│                                     │
└─────────────────────────────────────┘
```

| Button | Funktion | Beschreibung |
|--------|----------|-------------|
| **Device ID** | ID-Editor | 4-stellige Geräte-ID mit Touch-Numpad ändern |
| **→ Portrait/Landscape** | Orientierung | Sofortige Umschaltung Portrait ↔ Landscape |
| **WiFi ON/OFF** | Access Point | WLAN-Hotspot für Web-Konfiguration |
| **Web Config** | Browser-Interface | Öffnet Web-Interface (http://192.168.4.1) |
| **SAVE & EXIT** | Speichern | Konfiguration in EEPROM speichern und verlassen |
| **CANCEL** | Abbrechen | Verlassen ohne Speichern |

### Device ID Editor

```
┌─────────────────────────────────────┐
│          DEVICE ID EDITOR           │
├─────────────────────────────────────┤
│        Current: [5][9][9][9]        │
│                  ↑ Position 1/4     │
│                                     │
│  [1] [2] [3]     [+]                │
│  [4] [5] [6]     [-]                │
│  [7] [8] [9]     [<] [>]            │
│      [0]         [OK] [CANCEL]      │
│                                     │
└─────────────────────────────────────┘
```

| Touch-Button | Funktion |
|-------------|----------|
| **0-9** | Ziffer setzen + automatisch weiter |
| **+** | Aktuelle Ziffer +1 (0-9 Zyklus) |
| **-** | Aktuelle Ziffer -1 (9-0 Zyklus) |
| **< >** | Position links/rechts |
| **OK** | Device ID übernehmen |
| **CANCEL** | Abbrechen ohne Änderung |

### WiFi Access Point

**Aktivierung**: Service-Menü → "WiFi ON" oder `ý5999.SYS.1.WIFI.1þ`

| Parameter | Wert |
|-----------|------|
| **SSID** | ESP32-ServiceMode |
| **Passwort** | service123 |
| **IP-Adresse** | 192.168.4.1 |
| **Web-Interface** | http://192.168.4.1 |

## ⚡ Funktionen

### Hauptfunktionen (Version 1.50)
- **6-Tasten Touch-Interface** im 3x2 (Landscape) oder 2x3 (Portrait) Layout
- **PWM-gesteuerte Hintergrundbeleuchtung** mit 256 Stufen (0-100%)
- **RGB-LED Feedback** für visuelle Rückmeldung bei Kommunikation
- **CSMA/CD RS485-Kommunikation** für kollisionsfreie industrielle Netzwerke
- **Service-Menü** mit Touch- und Telegramm-Aktivierung
- **Device ID Konfiguration** über Touch-Numpad
- **Orientierungs-Umschaltung** mit sofortiger Anwendung
- **WiFi Access Point** für Web-basierte Konfiguration
- **EEPROM-Speicherung** für persistente Einstellungen
- **Touch-Kalibrierung** über integrierten Testmodus

### CSMA/CD Features
- **Carrier Sense**: Lauscht auf Bus vor dem Senden
- **Collision Detection**: Erkennt Kollisionen durch Datenvergleich  
- **Sendepuffer**: Prioritätsbasierte Warteschlange (10 Telegramme)
- **Backoff-Algorithmus**: Exponentielles Warten bei Kollisionen
- **Automatische Wiederholung**: Bis zu 5 Versuche pro Telegramm
- **Statistiken**: Überwachung von Sendungen, Kollisionen, Retries

### Button-Funktionalität
- **Visuelle Rückmeldung**: Buttons wechseln die Farbe bei Berührung
- **Zustandsspeicherung**: Aktive Buttons bleiben grün markiert
- **Protokoll-Integration**: Sendet BTN-Telegramme bei Berührung/Loslassen
- **LED-Zuordnung**: LED 49-54 steuern Button 1-6

## ⚙️ Konfiguration

### Display-Konfiguration (config.h)

```cpp
// Display-Dimensionen und Ausrichtung
#define PORTRAIT 0
#define LANDSCAPE 1
#define SCREEN_ORIENTATION LANDSCAPE  // Kann über Service-Menü geändert werden

// Hintergrundbeleuchtungs-Pin und Kanal
#define TFT_BL_PIN 27
#define TFT_BL_CHANNEL 0  // PWM-Kanal (0-15)
#define DEFAULT_BACKLIGHT 100  // Standardhelligkeit (0-100%)
```

### Service-Manager Konfiguration (config.h)

```cpp
// Service-Menü Parameter
#define SERVICE_TOUCH_TIME 20000     // 20 Sekunden für Aktivierung
#define SERVICE_EEPROM_ADDR 100      // EEPROM-Startadresse
#define SERVICE_WIFI_SSID "ESP32-ServiceMode"
#define SERVICE_WIFI_PASSWORD "service123"
```

### CSMA/CD Konfiguration (config.h)

```cpp
// CSMA/CD Parameter
#define BUS_IDLE_TIME 10             // Zeit ohne Aktivität = Bus frei (ms)
#define COLLISION_DETECT_TIME 5      // Zeit für Kollisionsprüfung (ms)
#define SEND_QUEUE_SIZE 10           // Sendepuffer-Größe
#define MAX_RETRIES_PER_TELEGRAM 5   // Maximale Wiederholungen
```

## 📡 Kommunikationsprotokoll

### Telegramm-Format

Alle Kommunikation erfolgt über strukturierte Telegramme:

```
[START_BYTE] DEVICE_ID.FUNCTION.INSTANCE_ID.ACTION.PARAMS [END_BYTE]
```

**Beispiel**: `ý5999.LBN.16.SET_MBR.75þ` setzt die Hintergrundbeleuchtung auf 75%.

### UART-Konfiguration

| Parameter | Wert | Beschreibung |
|-----------|------|-------------|
| **Baudrate** | 57600 | Übertragungsgeschwindigkeit |
| **Datenbits** | 8 | Anzahl Datenbits |
| **Parität** | Even (Gerade) | Fehlerprüfung |
| **Stoppbits** | 1 | Anzahl Stoppbits |
| **Format** | 8E1 | Zusammenfassung |

### Verfügbare Befehle (Version 1.50)

#### System-Befehle (NEU)

| Befehl | Beschreibung | Beispiel |
|--------|-------------|----------|
| `SYS.1.SERVICE.1/0` | Service-Modus ein/aus | `ý5999.SYS.1.SERVICE.1þ` |
| `SYS.1.WIFI.1/0` | WiFi Access Point ein/aus | `ý5999.SYS.1.WIFI.1þ` |
| `SYS.1.WEBSERVER.1/0` | Web-Server ein/aus | `ý5999.SYS.1.WEBSERVER.1þ` |
| `SYS.1.RESET.0` | ESP32 Neustart | `ý5999.SYS.1.RESET.0þ` |

#### Empfangene Befehle (ESP32 als Empfänger)

| Befehl | Beschreibung | Beispiel | Antwort |
|--------|-------------|----------|---------|
| `LBN.16.SET_MBR.{0-100}` | Hintergrundbeleuchtung einstellen | `ý5999.LBN.16.SET_MBR.75þ` | Helligkeit → 75% |
| `LED.{49-54}.ON.{0-100}` | Button aktivieren mit Helligkeit | `ý5999.LED.49.ON.100þ` | Button 1 → weiß hell |
| `LED.{49-54}.ON.0` | Button deaktivieren | `ý5999.LED.49.ON.0þ` | Button 1 → grau |
| `LED.{49-54}.OFF.0` | Button deaktivieren (alternativ) | `ý5999.LED.50.OFF.0þ` | Button 2 → grau |

#### Gesendete Befehle (ESP32 als Sender)

| Befehl | Beschreibung | Wann gesendet | Beispiel |
|--------|-------------|---------------|----------|
| `BTN.{17-22}.STATUS.1` | Button gedrückt | Bei Touch-Ereignis | `ý5999.BTN.17.STATUS.1þ` |
| `BTN.{17-22}.STATUS.0` | Button losgelassen | Nach Touch-Ereignis | `ý5999.BTN.17.STATUS.0þ` |
| `LBN.16.STATUS.{0-100}` | Helligkeitsstatus | Alle 23 Sekunden | `ý5999.LBN.16.STATUS.100þ` |

### Button-LED-Zuordnung (Korrigiert)

| Button-Position | Button-ID | LED-ID | Landscape Layout | Portrait Layout |
|----------------|-----------|--------|------------------|-----------------|
| Button 1 | BTN.17 | LED.49 | Oben Links | Oben Links |
| Button 2 | BTN.18 | LED.50 | Oben Mitte | Oben Rechts |
| Button 3 | BTN.19 | LED.51 | Oben Rechts | Mitte Links |
| Button 4 | BTN.20 | LED.52 | Unten Links | Mitte Rechts |
| Button 5 | BTN.21 | LED.53 | Unten Mitte | Unten Links |
| Button 6 | BTN.22 | LED.54 | Unten Rechts | Unten Rechts |

## 🚀 Installation

### Voraussetzungen

1. **Arduino IDE** (Version 1.8.19 oder höher)
2. **ESP32 Board Package** für Arduino IDE (Version 3.2.0+)
3. **Erforderliche Bibliotheken**:
   - `TFT_eSPI` (Version 2.4.0 oder höher)
   - `XPT2046_Touchscreen`

### Bibliotheken installieren

```bash
# Arduino IDE → Tools → Manage Libraries
# Suchen und installieren:
TFT_eSPI by Bodmer
XPT2046_Touchscreen by Paul Stoffregen
```

### TFT_eSPI Konfiguration

1. Navigieren Sie zum TFT_eSPI Bibliotheksordner:
   ```
   Arduino/libraries/TFT_eSPI/
   ```

2. Ersetzen Sie `User_Setup.h` mit der bereitgestellten Konfiguration:
   ```cpp
   #define ST7789_DRIVER
   #define TFT_WIDTH  240
   #define TFT_HEIGHT 320
   #define TFT_CS   15
   #define TFT_DC    2
   #define TFT_RST  -1
   // TFT_BL wird NICHT definiert (für eigene PWM-Steuerung)
   #define TFT_MOSI 13
   #define TFT_SCLK 14
   #define TFT_MISO 12
   #define TOUCH_CS 33
   ```

### Projekt-Setup (Version 1.50)

1. **Projektverzeichnis erstellen**:
   ```
   ESP32_CYD_RS485_HAUS_BUS_V150/
   ├── ESP32_CYD_RS485_HAUS-BUS.ino
   ├── config.h
   ├── communication.h
   ├── communication.cpp
   ├── service_manager.h      # NEU
   ├── service_manager.cpp    # NEU
   ├── menu.h
   ├── menu.cpp
   ├── touch.h
   ├── touch.cpp
   ├── display.h
   ├── display.cpp
   ├── backlight.h
   ├── backlight.cpp
   ├── led.h
   ├── led.cpp
   └── User_Setup.h          # TFT_eSPI Konfiguration
   ```

2. **Kompilieren und Hochladen**:
   - Board: "ESP32 Dev Module"
   - Upload Speed: "921600"
   - CPU Frequency: "240MHz (WiFi/BT)"
   - Flash Frequency: "80MHz"
   - Flash Size: "4MB (32Mb)"

## 🎯 Quick-Setup Guide

### 1. Erste Inbetriebnahme
1. **Hardware verbinden** (RS485-Umsetzer, optional RGB-LED)
2. **Code kompilieren und hochladen**
3. **ESP32 startet** mit Standard-Konfiguration (Device ID: 5999, Landscape)

### 2. Service-Modus konfigurieren
1. **20 Sekunden Touch** auf beliebige Stelle → Service-Menü öffnet sich
2. **Device ID** → Numpad verwenden → 4-stellige ID eingeben → OK
3. **Orientierung** → Portrait/Landscape umschalten (sofortige Anwendung)
4. **SAVE & EXIT** → Konfiguration dauerhaft speichern

### 3. WiFi-Konfiguration (Optional)
1. **Service-Modus** → "WiFi ON" → Access Point startet
2. **Web Config** → IP-Adresse wird angezeigt (192.168.4.1)
3. **Browser öffnen** → http://192.168.4.1 → Web-Interface
4. **Remote-Konfiguration** über Webbrowser möglich

### 4. Telegramm-Steuerung testen
```bash
# System-Befehle
ý5999.SYS.1.SERVICE.1þ      # Service-Modus aktivieren
ý5999.SYS.1.WIFI.1þ         # WiFi einschalten
ý5999.SYS.1.RESET.0þ        # Neustart

# Backlight-Steuerung
ý5999.LBN.16.SET_MBR.50þ    # Helligkeit 50%

# LED-Steuerung (korrigierte IDs)
ý5999.LED.49.ON.100þ        # Button 1 → weiß hell
ý5999.LED.49.ON.0þ          # Button 1 → grau
```

## 🧪 Testmodus

### Testmodus aktivieren

1. Berühren Sie die **"TEST"**-Taste in der oberen rechten Ecke des Hauptmenüs
2. Der Testmodus öffnet sich mit Kalibrierungskreuzen und Kontrolltasten

### Verfügbare Testfunktionen

| Taste | Funktion | Beschreibung |
|-------|----------|-------------|
| **ESC** | Beenden | Zurück zum Hauptmenü |
| **INV-X** | X-Achse invertieren | Touchscreen-Kalibrierung X |
| **INV-Y** | Y-Achse invertieren | Touchscreen-Kalibrierung Y |
| **BL+** | Helligkeit erhöhen | +10% Hintergrundbeleuchtung |
| **BL-** | Helligkeit verringern | -10% Hintergrundbeleuchtung |

## 🐛 Debugging

### Debug-Ausgaben (Version 1.50)

```cpp
// In config.h aktivieren/deaktivieren:
#define DB_TX_HEX 0      // Zeigt gesendete Telegramme in Hex
#define DB_RX_HEX 0      // Zeigt empfangene Bytes in Hex
#define DB_RX_INFO 0     // Zeigt Telegramm-Struktur
#define DB_INFO 0        // Service-Manager Debug-Ausgaben
```

### Typische Debug-Ausgaben

```
ServiceManager initialisiert - Version 1.50
Konfiguration geladen - Device ID: 5999, Orientation: LANDSCAPE
Service-Aktivierung gestartet (20 Sekunden)
Service-Aktivierung 50%
Service-Modus aktiviert
LED 49 (Button 1) aktiviert - Weiß mit Helligkeit 100%
WiFi AP gestartet - SSID: ESP32-ServiceMode
Web-Server gestartet (vereinfacht)
```

## 🔧 Troubleshooting

### Service-Menü Probleme

#### Service-Modus startet nicht
```cpp
// Debug aktivieren:
#define DB_INFO 1

// Überprüfen:
- 20 Sekunden ununterbrochen berühren
- Progress-Bar muss 100% erreichen
- Alternativ: Telegramm ý5999.SYS.1.SERVICE.1þ senden
```

#### Device ID wird nicht gespeichert
```cpp
// EEPROM-Status prüfen:
EEPROM.begin(512);  // Ausreichend Speicher reserviert?

// Debug-Ausgabe:
Serial.println("Konfiguration gespeichert");
```

#### WiFi funktioniert nicht
```cpp
// WiFi-Status prüfen:
if (WiFi.getMode() == WIFI_AP) {
  Serial.println("AP-Modus aktiv");
  Serial.println(WiFi.softAPIP());
}

// Manuell testen:
WiFi.softAP("ESP32-ServiceMode", "service123");
```

### Kommunikationsprobleme (CSMA/CD)

#### Häufige Kollisionen
```cpp
// CSMA/CD Parameter anpassen:
#define BUS_IDLE_TIME 20        // Längere Wartezeit
#define COLLISION_DETECT_TIME 10 // Längere Kollisionserkennung

// Statistiken prüfen:
printCommunicationStats();
```

#### Sendepuffer läuft voll
```cpp
// Puffer-Größe erhöhen:
#define SEND_QUEUE_SIZE 20

// Prioritäten optimieren:
- Kritische Nachrichten: Priorität 0-1
- Normal: Priorität 5
- Status: Priorität 7-9
```

### LED-Button-Zuordnung

#### Falsche LED reagiert
```cpp
// Korrekte Zuordnung prüfen:
LED.49 → Button 1 (Index 0)
LED.50 → Button 2 (Index 1)
LED.51 → Button 3 (Index 2)
LED.52 → Button 4 (Index 3)
LED.53 → Button 5 (Index 4)
LED.54 → Button 6 (Index 5)

// Debug-Ausgabe:
Serial.print("LED ID: "); Serial.print(ledId);
Serial.print(" → Button Index: "); Serial.println(buttonIndex);
```

## 📊 Performance & Statistiken

### CSMA/CD Monitoring
```cpp
// Statistiken alle 60 Sekunden ausgeben:
printCommunicationStats();

// Ausgabe-Beispiel:
=== Kommunikations-Statistiken ===
Gesendete Telegramme: 42
Erkannte Kollisionen: 3
Wiederholungen: 5
Sendepuffer-Status: 2/10
Bus-Status: Frei
```

### Service-Manager Status
```cpp
// Service-Menü Informationen:
- Device ID: 5999
- Orientierung: LANDSCAPE  
- WiFi: Aktiv (192.168.4.1)
- Konfiguration: Gespeichert
```

## 📄 Dokumentation

- **[CHANGELOG.md](CHANGELOG.md)** - Versionshistorie und Änderungen
- **[User_Setup.h](User_Setup.h)** - TFT_eSPI Konfiguration
- **[Schaltplan/Pinout]** - Hardware-Dokumentation

## 📝 Lizenz

Dieses Projekt steht unter der MIT-Lizenz. Weitere Details finden Sie in der [LICENSE](LICENSE) Datei.

## 🤝 Beitrag

Beiträge sind willkommen! Bitte lesen Sie [CONTRIBUTING.md](CONTRIBUTING.md) für Details zum Vorgehen.

## 📞 Support

Bei Fragen oder Problemen:
1. Überprüfen Sie zuerst die [Troubleshooting](#troubleshooting)-Sektion
2. Aktivieren Sie Debug-Ausgaben für detaillierte Informationen
3. Dokumentieren Sie das Problem mit Serial Monitor Ausgaben
4. Erstellen Sie ein GitHub Issue mit allen relevanten Informationen

---

**Version**: 1.50  
**Datum**: Mai 2025  
**Kompatibilität**: ESP32, Arduino IDE 1.8.19+, TFT_eSPI 2.4.0+  
**Hardware**: TZT ESP32 2.4" LCD (ST7789 + XPT2046)  
**Features**: Service-Menü, WiFi AP, CSMA/CD, EEPROM-Config
