
***

# SYSTEM DOCUMENTATION & OPERATIONAL MANUAL
**Project Title:** Design and Implementation of a Low-Power IoT-Based Smart Women’s Safety Device with Heart-Rate Monitoring and Real-Time Alert System.
**Developer:** Ugwoke Chizaram Precious (21CK029347)
**Institution:** Covenant University, Electrical and Electronics Engineering

---

## 1. INTRODUCTION AND SYSTEM ARCHITECTURE
This project is an advanced, hybrid (Offline/Online) IoT personal safety device designed specifically to protect vulnerable individuals through dual-trigger emergency mechanisms: Manual (Push Button) and Automatic (Biometric Heart Rate Sensing). 

Unlike conventional safety devices that rely purely on microcontrollers running sequential loops, this system utilizes a **Non-Blocking Multitasking Architecture** built on the ESP32. This means the device can simultaneously listen to a user’s pulse, fetch GPS coordinates from satellites, update an OLED screen, and sync with a Firebase cloud database without any single process freezing or halting the others. 

### 1.1 Hardware Hookup & Pin Definitions
The system revolves around the ESP32 microcontroller acting as the brain. The peripherals are connected as follows:

*   **NEO-6M GPS Module (Location Tracking):** Connected to hardware **UART2**. 
    *   `GPS_RX_PIN = 16` (Connects to GPS TX)
    *   `GPS_TX_PIN = 17` (Connects to GPS RX)
    *   *Baud Rate:* 115200 for fast, reliable data acquisition.
*   **SIM800L GSM Module (SMS Communications):** Connected to hardware **UART1**.
    *   `GSM_RX_PIN = 25` (Connects to GSM TX)
    *   `GSM_TX_PIN = 33` (Connects to GSM RX)
    *   *Note:* Standard UART1 pins on the ESP32 often conflict with internal flash memory. They were intelligently remapped to GPIO 25 and 33 to guarantee system stability and prevent boot failures.
*   **DFRobot PPG Heart Rate Sensor:** Connected to **Analog Pin 36**.
    *   Because the ESP32 natively uses a 12-bit ADC (Analog-to-Digital Converter), the code explicitly forces a 10-bit resolution (`analogReadResolution(10);`) so that the mathematical thresholds defined in the library (Threshold = 550) operate perfectly.
*   **1.3" SH1106 OLED Display:** Connected via **I2C** standard pins (SDA = 21, SCL = 22).
*   **Tactile Push Button:** Connected to **Pin 5** using the ESP32's internal Pull-Up resistor (`INPUT_PULLUP`). 
*   **Active Buzzer:** Connected to **Pin 18**.
*   **LED Indicators:** 
    *   *WiFi/Cloud Status (Green):* **Pin 19**
    *   *GPS Lock Status (Yellow):* **Pin 4**

---

## 2. SOFTWARE LOGIC AND CORE FUNCTIONALITIES

### 2.1 The Non-Blocking Core (`millis()` over `delay()`)
The most crucial design choice in this software is the absolute elimination of the `delay()` function inside the main operational loop. Using `delay()` would cause the ESP32 to literally stop processing for that duration, which means it could miss a critical heartbeat spike or a chunk of GPS satellite data. Instead, the system uses "State Machines" and `millis()` to track time. Every component checks the current system time, determines if it is its turn to execute, executes in a fraction of a millisecond, and yields the processor back.

### 2.2 System Bootup Sequence
When powered on, the system undergoes a rigorous diagnostic and initialization sequence:
1.  **Hardware Init:** Pins are set, and peripherals are turned off to prevent premature buzzing.
2.  **Display Boot:** The OLED shows a personalized splash screen ("SMART SAFETY DEVICE - UGWOKE CHIZARAM P.") for 3 seconds.
3.  **Serial Communications:** The GPS and GSM modules are booted up. The GSM module is sent an AT command (`AT+CMGF=1`) forcing it into Text Mode for SMS capabilities.
4.  **Network Attempt:** The device attempts to connect to the predefined WiFi network. *Intelligent Fallback:* It only attempts connection for a maximum of 20 iterations (roughly 6 seconds). If no WiFi is found, it does not freeze; it abandons the attempt and successfully boots into **Offline Mode**.
5.  **Firebase & NTP:** If WiFi is available, it connects to Google Firebase and queries an NTP server to grab the current exact date and time for Nigeria (UTC+1).

### 2.3 The Dual Alert Trigger System
The system can be pushed into "Danger Mode" in two independent ways:

**A. Manual Trigger (Push Button)**
The button utilizes software "debouncing" (a 50-millisecond filter) to ensure that mechanical electrical noise isn't registered as a false press. 
*   **Short Press (Panic):** Pressing the button for less than 2 seconds immediately triggers Danger Mode.
*   **Long Press (Safe Mode):** Pressing and holding the button for exactly 2 seconds or more acts as a cancellation. It resets the system to Safe Mode, halts the buzzer, and dispatches a "False alarm/I am safe" message.

**B. Automatic Trigger (Biometric Heart Rate)**
The DFRobot PPG sensor utilizes a highly intelligent 3-tier State Machine to prevent false positives (like accidental finger movement triggering a panic alert):
1.  **State 1: HR_IDLE:** No finger is detected. The system ignores all background light/noise.
2.  **State 2: HR_STABILIZING:** The moment a finger is placed, the system enters this state. Because PPG sensors are incredibly sensitive to capillary blood flow adjustments, the system will completely ignore all BPM numbers for **18 solid seconds**. The screen will read "Reading...".
3.  **State 3: HR_MONITORING:** After 18 seconds, the baseline is established, and active monitoring begins. 
    *   *The Anomaly Filter:* If the BPM drops below 50 or spikes above 120, it counts as an anomaly. However, to account for sensor glitches, it requires **5 consecutive abnormal beats** to trigger Danger Mode. Once 5 anomalous beats are recorded sequentially, the system auto-triggers the emergency sequence without the user ever touching a button.
    *   *Smart Lockout:* If the system is already in Danger Mode, the heart rate sensor intelligently stops analyzing anomalies to prevent the system from getting stuck in an infinite alert loop.

### 2.4 Smart Buzzer Operations
A continuous loud buzzer can be highly annoying and chaotic during an emergency. The system features a "Smart Buzzer Pattern." When an alert is triggered, the buzzer does not sound a flat continuous tone. Instead, it sounds three short, sharp bursts (Beep-Beep-Beep), followed by 3 seconds of total silence, and then repeats. This alerts bystanders to the emergency effectively without overwhelming the victim or the system operator.

### 2.5 Dynamic Cloud Synchronization (Firebase)
The system features two-way communication with the Firebase Realtime Database:
*   **Upstream (Telemetry Logging):** Every 6 seconds, the system packages the user's current Heart Rate, Alert Status (SAFE/DANGER), and a precise Google Maps URL of their Latitude and Longitude, pushing it to Firebase. A remote guardian can watch this database update in real time.
*   **Downstream (Dynamic Contact Update):** Every 10 seconds, the device reads the `/Phone_Number` node from the database. This allows a guardian to change the emergency contact number remotely from anywhere in the world. 
    *   *Number Formatting & UI Sync:* When a new number is detected (e.g., entered as `+2348163180829` or `2348163180829` in Firebase), the ESP32 processes the string, strips the country code, and replaces it with a local `0`. Furthermore, it halts the main OLED dashboard and triggers a non-blocking popup overlay ("NEW CONTACT SYNCED") showing the new formatted number for exactly 3 seconds, proving to the user that the remote update was successful.

### 2.6 SMS Dispatching Logic
When an alert is triggered, a highly formatted SMS is built. It includes:
1.  The cause of the alert ("Manual Panic Button Pressed!" or "Auto-Alert: Abnormal Heart Rate (160 BPM)!").
2.  The exact real-time Google Maps Link. (If GPS is unavailable due to being deep indoors, it gracefully falls back to stating "GPS is currently unavailable").
3.  The exact Date and Time stamp fetched from the NTP internet servers.
While sending the SMS, the OLED temporarily displays "SENDING SMS... Please Wait." so the user is visually aware that communication is in progress.

---

## 3. OLED DASHBOARD USER INTERFACE GUIDE
The 1.3" OLED screen acts as the central command dashboard. It is designed to display all critical information concurrently without forcing the user to wait for rotating screens.

**Top Row (System Status & Connectivity):**
*   `MODE: SAFE` or `MODE: DANGER!` dictates the current alert condition of the device.
*   `[WIFI]` appears on the top right if connected to the cloud. `[X-FI]` appears if running purely on hardware offline mode.

**Middle Row (Vitals):**
*   `HR: -- (Place Finger)`: System is idle.
*   `HR: 85 (Reading...)`: Finger placed, undergoing the 18-second stabilization grace period.
*   `HR: 72 BPM (Active)`: Fully stabilized, actively monitoring for drops < 50 or spikes > 120.

**Bottom Rows (Geolocation):**
*   `GPS: SEARCHING...`: Trying to lock onto satellites. `Lat: --` and `Lng: --`.
*   `GPS: LOCKED`: Satellites found. Displays real-time Latitude and Longitude to 5 decimal places.

---

## 4. HOW TO OPERATE THE DEVICE (STEP-BY-STEP)
1.  **Power On:** Supply power to the device (via battery/USB). Watch the boot screen.
2.  **Wait for Connections:** Allow the device roughly 30 seconds to initialize the GSM network and attempt WiFi connection. If the Green LED lights up, it is connected to Firebase. 
3.  **GPS Lock:** Take the device near a window or outdoors. Once satellites are found, the Yellow LED will illuminate, and coordinates will appear on the screen.
4.  **Heart Rate Monitoring:** Place your index finger gently on the DFRobot pulse sensor. 
    *   *Do not press too hard*, as this restricts blood flow. 
    *   Hold perfectly still for 18 seconds while the screen says "Reading...". 
    *   Once it switches to "Active", the safety net is armed.
5.  **Triggering an Emergency:**
    *   *Manual:* Tap the push button quickly.
    *   *Automatic:* If your heart rate goes to an extreme (e.g., simulating a spike by tapping the sensor quickly, or naturally if stressed), wait for 5 consecutive extreme readings.
6.  **During an Emergency:** The buzzer will perform its Beep-Beep-Beep pattern. The screen will read `MODE: DANGER!`. An SMS will be sent to the configured number, and Firebase will log the Danger status.
7.  **Resetting to Safe Mode:** To declare the emergency over, press and hold the button for at least 2 full seconds until the buzzer stops. The system will send a "False alarm/Safe" SMS and return to normal monitoring.
8.  **Remote Configuration:** Open the Firebase console on a laptop or phone, go to Realtime Database, and change the string value under `/Phone_Number`. Watch the physical device's OLED screen react immediately with the "NEW CONTACT SYNCED" popup overlay.

---

## 5. WHAT TO EXPECT & SYSTEM LIMITATIONS
It is vital for the operator and evaluators to understand the physical constraints of IoT devices to accurately judge system performance.

*   **GPS Limitations:** The NEO-6M requires a direct line-of-sight to the sky. **Do not expect it to lock onto coordinates deep inside a concrete building or basement.** If it cannot lock, the SMS will intelligently adapt and send a "GPS Unavailable" message rather than failing to send altogether.
*   **Heart Rate Sensor Sensitivity:** Optical PPG sensors are notoriously sensitive to movement and ambient light. **Expect erratic readings if the finger is moving, pressing too hard, or held under direct sunlight.** This is exactly why the 18-second stabilization and 5-beat anomaly filter were heavily coded into the software—to combat hardware noise natively.
*   **GSM Network Delays:** Sending an SMS over a 2G/GSM network is not instantaneous. After an alert triggers, expect a 5 to 8-second delay before the SMS dispatch sequence fully completes and arrives at the recipient's phone.
*   **Offline Resilience:** If the WiFi router dies, **expect the device to function perfectly.** The Firebase data will fail to upload, but the core objective of the project (Personal Safety) remains completely intact. The heart rate sensor will continue to monitor, the buzzer will still sound, and the GSM module will still send the SMS relying purely on the cellular network.

---
### CONCLUSION
This system represents a highly resilient, fault-tolerant approach to personal safety. By combining local hardware interrupts, biological sensing algorithms, and real-time cloud computing into a non-blocking multitasker, it effectively bridges the gap between basic embedded electronics and modern IoT infrastructure.