# IoT-Based Smart Women's Safety Device with Heart-Rate Monitoring and Real-Time Alert System

A hybrid online/offline IoT personal safety device designed for women, built around an **ESP32** microcontroller. It combines biometric PPG heart-rate monitoring, real-time GPS tracking, SIM800L GSM SMS alerting, and Firebase cloud telemetry into a single self-contained battery-powered enclosure.

---

## Portfolio Page

Full project documentation, methodology, circuit diagrams, 75 build photos, firmware walkthrough, and live demonstration:

- **Embedded Systems page:** https://damilareadekeye.com/works/embedded-systems/smart-women-safety-device-dfrobot-bpm/

---

## How It Works

```
User places finger on DFRobot PPG sensor
         |
         v
HR_IDLE → HR_STABILIZING (18-second grace period — BPM displayed, no anomaly counting)
         |
         v (after 18 s)
HR_MONITORING: active anomaly detection
BPM < 50 or > 120  →  abnormalBeatCount++
5 consecutive abnormal beats   →  triggerDanger("Auto-Alert: Abnormal Heart Rate")
         |
         v (OR: user short-presses panic button)
triggerDanger(cause) — also callable directly by button
         |
         v
Acquire GPS coordinates (NEO-6M, UART2, TinyGPS++)
Build Google Maps URL: https://maps.google.com/?q=LAT,LNG
Get NTP timestamp (pool.ntp.org, UTC+1 Nigeria)
         |
         v
SIM800L GSM: AT+CMGF=1 → AT+CMGS → send SMS with cause + Maps URL + timestamp
Firebase RTDB: /Alert_Message = cause + location + timestamp
Buzzer: 3-beep → 3-second silence → repeat while DANGER active
OLED: MODE: DANGER! with live GPS rows
         |
         v (2-second long press on panic button)
triggerSafe() → alertActive = false → silence buzzer → send safe SMS
```

---

## Repository Structure

```
.
├── circuit-diagrams/
│   ├── Circuit_Diagram_Colour.pdf    # Full colour Proteus schematic
│   └── Circuit_Diagram_BW.pdf        # Black-and-white reference schematic
│
├── firmware/
│   ├── code_main/
│   │   └── code_main.ino             # Production ESP32 sketch (~600 lines)
│   └── reference-sketches/           # Individual component test sketches
│       ├── GSM_SIM800L_EVB_ESP32_ReMapped_UART1/
│       ├── OLED_test/
│       ├── ESP32_Heart_Pulse_Rate_BPM/
│       ├── ESP32_GPS_Neo6M/
│       └── LEDs_Pushbutton_Test/
│
└── docs/
    └── Knowledge_Base_and_Operational_Manual.md   # Full system documentation
```

---

## Hardware

| Component | Model / Spec | Role |
|-----------|-------------|------|
| Microcontroller | ESP32 Dev Board (30-pin) | Central processing, Wi-Fi, UART, I2C, ADC |
| Heart Rate Sensor | DFRobot Gravity PPG SEN0203 (analog) | Finger-clip optical PPG — GPIO 36, 10-bit ADC, threshold 550 |
| GPS Module | NEO-6M GY-GPS6MV2 | Real-time location — UART2 (GPIO 16/17), 115200 baud |
| GSM Module | SIM800L EVB with whip antenna | SMS dispatch — UART1 (GPIO 25/33), AT commands |
| OLED Display | SH1106 1.3" 128×64 I2C | Live dashboard via U8g2 library |
| Panic Button | Tactile push button (GPIO 5, INPUT_PULLUP) | Short press = danger; long press 2 s = safe |
| Buzzer | Active buzzer + BC547 NPN driver | 3-beep audible alert pattern |
| Status LEDs | Green (Wi-Fi, GPIO 19), Yellow (GPS, GPIO 4) | Visual connectivity indicators |
| LiPo Battery | 3.7 V LiPo | Primary portable power |
| Charger | LX-LCBST USB-C LiPo charger | Charging management |
| Boost Converter | XL6009 DC-DC step-up | 3.7 V → 5 V for ESP32 and peripherals |
| Prototyping PCB | 9 cm × 15 cm copper perfboard | Permanent soldered integration |
| Power Switch | 3-pin slide switch | Clean power on/off |

---

## GPIO Pin Assignments

| GPIO | Signal | Peripheral | Notes |
|------|--------|------------|-------|
| 16 | GPS_RX | NEO-6M TX | UART2 receive |
| 17 | GPS_TX | NEO-6M RX | UART2 transmit |
| 25 | GSM_RX | SIM800L TX | UART1 — remapped to avoid ESP32 flash conflict |
| 33 | GSM_TX | SIM800L RX | UART1 — remapped to avoid ESP32 flash conflict |
| 36 | PULSE_INPUT | DFRobot PPG output | Input-only ADC; 10-bit resolution |
| 2 | PULSE_BLINK | Onboard LED | Blinks on each detected heartbeat |
| 5 | BUTTON_PIN | Panic button | INPUT_PULLUP; active LOW; 50 ms debounce |
| 18 | BUZZER_PIN | Active buzzer via BC547 | HIGH = beep |
| 19 | WIFI_LED_PIN | Green status LED | HIGH when WiFi connected |
| 4 | GPS_LED_PIN | Yellow status LED | HIGH when GPS location valid |
| 21/22 | SDA/SCL | SH1106 OLED (I2C) | Default ESP32 I2C bus |

> **Note on UART1 remapping:** GPIO 9/10 (default UART1) are shared with the ESP32 flash memory chip and cause a crash if used as general UART. GPIO 25/33 are used instead.

---

## Heart Rate State Machine

```
HR_IDLE
  └─ (first beat detected)
     → HR_STABILIZING (18 seconds grace — anomaly counting suppressed)
          └─ (after 18 s)
             → HR_MONITORING
                  └─ BPM < 50 or > 120: abnormalBeatCount++
                       └─ (5 consecutive abnormal) → triggerDanger()
                  └─ BPM 50–120: abnormalBeatCount = 0 (reset)

  Any state: no beat for 7 seconds → HR_IDLE (finger removed)
  DANGER mode active: anomaly analysis suspended (prevents infinite loop)
```

---

## Firebase Database Structure

| Path | Type | Description |
|------|------|-------------|
| `/Telemetry` | JSON | BPM, Status, Latitude, Longitude, LocationURL — updated every 6 s |
| `/Phone_Number` | String | Emergency contact number — read every 10 s; changeable remotely |
| `/Alert_Message` | String | Written on every danger/safe event with cause + location + timestamp |

---

## Quick Start

### 1. Flash the ESP32

Open `firmware/code_main/code_main.ino` in Arduino IDE (ESP32 board support installed).

**Required libraries** (install via Library Manager):
- `TinyGPS++`
- `U8g2`
- `PulseSensorPlayground`
- `FirebaseESP32`
- `NTPClient`

Update the Wi-Fi credentials and Firebase database URL in the sketch before uploading:

```cpp
#define WIFI_SSID      "Your_Network_Name"
#define WIFI_PASSWORD  "Your_Password"
#define DATABASE_URL   "https://your-project-default-rtdb.firebaseio.com/"
#define LEGACY_TOKEN   "your_firebase_legacy_token"
```

### 2. Test individual components first

Use the reference sketches in `firmware/reference-sketches/` to verify each component independently before running the full integration sketch:

| Sketch | Component to test |
|--------|------------------|
| `GSM_SIM800L_EVB_ESP32_ReMapped_UART1` | SIM800L on GPIO 25/33 |
| `OLED_test` | SH1106 1.3" OLED via U8g2 |
| `ESP32_Heart_Pulse_Rate_BPM` | DFRobot PPG sensor on GPIO 36 |
| `ESP32_GPS_Neo6M` | NEO-6M GPS on UART2 |
| `LEDs_Pushbutton_Test` | LED indicators and panic button |

### 3. Set up Firebase

In Firebase Console:
1. Create a Realtime Database
2. Add the path `/Phone_Number` with the emergency contact in international format (e.g., `+2348163180829`)
3. Copy the database URL and legacy token into the sketch

### 4. Boot sequence

On power-on:
1. OLED shows "SMART SAFETY DEVICE / UGWOKE CHIZARAM P." splash for 3 seconds
2. GPS and GSM modules initialise; GSM sent `AT+CMGF=1` (text mode)
3. Device attempts Wi-Fi connection (max 20 attempts ≈ 6 seconds)
4. If connected: Firebase initialises, NTP time synced
5. If no Wi-Fi: device boots into offline mode — GPS + GSM still fully operational

---

## Docs

`docs/Knowledge_Base_and_Operational_Manual.md` contains the complete system documentation including the non-blocking multitasking architecture, dual-trigger logic deep-dive, OLED dashboard UI guide, Firebase sync detail, and operating instructions.

---

*Hardware: ESP32 + DFRobot Gravity PPG SEN0203 + NEO-6M GPS + SIM800L GSM + SH1106 OLED*  
*Firebase RTDB + NTPClient — Hybrid online/offline operation*
