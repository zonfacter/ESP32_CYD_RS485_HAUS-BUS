22.05.2025 14:15
# 📋 Änderungen zur GitHub Version - Kurzzusammenfassung

## 🔧 1. LED-Button-Zuordnung korrigiert
- **Problem**: LED-IDs 17-22 wurden erwartet, aber Protokoll nutzt LED-IDs 49-54
- **Lösung**: 
  - `communication.cpp`: LED-Verarbeitung von `17-22` auf `49-54` geändert
  - `menu.cpp`: Button instanceIDs von `"1-6"` auf `"17-22"` geändert
  - `ESP32_CYD_RS485_HAUS-BUS.ino`: Debug-Ausgaben für Zuordnung erweitert

## 🎨 2. LED-Helligkeitssteuerung korrigiert
- **Problem**: `LED.49.ON.0` setzte Button trotzdem auf aktiv (grün)
- **Lösung**: `communication.cpp` - Bei Helligkeit = 0 wird Button auf grau/inaktiv gesetzt
- **Verhalten**: 
  - `ON.100` → Weiß hell (aktiv)
  - `ON.0` → Grau (inaktiv)
  - `OFF` → Grau (inaktiv)

## 💡 3. Hintergrundbeleuchtung repariert
- **Problem**: TFT_eSPI überschrieb PWM auf Pin 27 mit digitalWrite()
- **Lösung**:
  - `User_Setup.h`: TFT_BL Definitionen auskommentiert
  - `backlight.cpp`: Vereinfacht, exakt wie funktionierender Test-Sketch

## 🗂️ Geänderte Dateien:
```
✅ communication.cpp            - LED-Zuordnung + Helligkeit
✅ menu.cpp                     - Button instanceIDs 
✅ ESP32_CYD_RS485_HAUS-BUS.ino - Debug-Ausgaben
✅ backlight.cpp                - PWM-Steuerung vereinfacht
✅ User_Setup.h                 - TFT_eSPI Backlight deaktiviert
```

## 🎯 Endergebnis:
- **LED 49-54** steuern korrekt **Button 1-6**
- **Helligkeitssteuerung** funktioniert (0=grau, 1-100=weiß gedimmt)
- **Hintergrundbeleuchtung** reagiert auf SET_MBR Befehle
- **CSMA/CD** bleibt unverändert funktionsfähig

**Kern-Problem war die falsche LED-ID-Zuordnung und TFT_eSPI-Konflikt beim Backlight-Pin!** 🚀

## 📊 Zuordnungstabelle:
| Button | BTN-ID | LED-ID | Beschreibung |
|--------|--------|--------|--------------|
| Button 1 | BTN.17 | LED.49 | Taster 1 ↔ LED 1 |
| Button 2 | BTN.18 | LED.50 | Taster 2 ↔ LED 2 |
| Button 3 | BTN.19 | LED.51 | Taster 3 ↔ LED 3 |
| Button 4 | BTN.20 | LED.52 | Taster 4 ↔ LED 4 |
| Button 5 | BTN.21 | LED.53 | Taster 5 ↔ LED 5 |
| Button 6 | BTN.22 | LED.54 | Taster 6 ↔ LED 6 |

## 🧪 Test-Befehle:
```
ý5999.LED.49.ON.100þ    # Button 1 → Weiß hell
ý5999.LED.49.ON.0þ      # Button 1 → Grau (aus)
ý5999.LBN.16.SET_MBR.50þ # Backlight → 50%
```

## 🔄 CSMA/CD Features (unverändert):
- ✅ Carrier Sense Multiple Access
- ✅ Collision Detection 
- ✅ Prioritätsbasierter Sendepuffer
- ✅ Exponentieller Backoff-Algorithmus
- ✅ Automatische Wiederholung (bis 5 Versuche)
- ✅ Statistiken und Monitoring

## 📱 Hardware-Konfiguration:
- **Board**: TZT ESP32 2.4" LCD (ST7789 + XPT2046)
- **RS485**: UART2 (RX=21, TX=22) mit CSMA/CD
- **Baudrate**: 57600 bps, 8E1
- **Device-ID**: 5999

---

**Version**: 2.4 (Mai 2025)  
**Status**: Vollständig funktionsfähig  
**Kompatibilität**: ESP32, Arduino IDE 1.8.19+
