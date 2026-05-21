/*
 * PROJECT: Design and Implementation of a Low-Power IoT-Based Smart Women’s Safety Device
 * WITH Heart-Rate Monitoring and Real-Time Alert System.
 * STUDENT: UGWOKE CHIZARAM PRECIOUS (21CK029347)
 * 
 * PHASE 2: Full Integration - Final UI Fixes, Smart Buzzer & Dynamic Phone Overlay
 * - GPS (NEO-6M) on UART2 (Baud 115200)
 * - GSM (SIM800L) on UART1 (Baud 115200)
 * - Pulse Sensor (DFRobot) with 18s Stabilization
 * - Push Button (Short Press = Danger, Long Press = Safe)
 * - 1.3" I2C OLED (U8g2 Library) Unified Dashboard + Non-blocking Overlays
 * - WiFi + Firebase RTDB + NTP Time (Nigeria UTC+1)
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <PulseSensorPlayground.h>

// IoT Libraries
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================
// GPS Module (UART2)
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// GSM Module (UART1)
#define GSM_RX_PIN 25
#define GSM_TX_PIN 33

// Heart Rate Sensor
#define PULSE_INPUT 36
#define PULSE_BLINK 2
#define THRESHOLD 550

// Peripherals
#define BUTTON_PIN 5
#define BUZZER_PIN 18
#define WIFI_LED_PIN 19   
#define GPS_LED_PIN 4

// ==========================================
// WIFI & FIREBASE CONFIGURATION
// ==========================================
#define WIFI_SSID "Network_Connect"
#define WIFI_PASSWORD "vvvvvvvv"
#define DATABASE_URL "https://women-bpm-safety-gps-tracker-default-rtdb.firebaseio.com/"
#define LEGACY_TOKEN "29NEbel8hJHqqwwovVOLSV90JELQzmCnVhk9b8wu"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// NTP Configuration (Nigeria Time, UTC+1 = 3600 seconds)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// ==========================================
// OBJECT INITIALIZATIONS
// ==========================================
TinyGPSPlus gps;
HardwareSerial GSM_SERIAL(1);
HardwareSerial GPS_SERIAL(2);
PulseSensorPlayground pulseSensor;
// Page buffer initialization for SH1106 1.3" OLED
U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// ==========================================
// SYSTEM VARIABLES
// ==========================================
String recipientNumber = "+2348163180829"; 
double latitude = 0.0, longitude = 0.0;
bool gpsValid = false;
bool lastGpsValidState = false; 
String locationURL = "";
String currentTimestamp = "";

// Alert & Buzzer Variables
bool alertActive = false;
unsigned long lastBuzzerTime = 0;
bool buzzerState = false;
int beepCount = 0;
bool inSilencePeriod = false;

// UI Overlay Variables (For New Phone Number display)
bool showNewContactOverlay = false;
unsigned long newContactOverlayTime = 0;
String displayContactNumber = "";

// Button Debounce & Timing Variables
int stableButtonState = HIGH;
int lastRawButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long LONG_PRESS_TIME = 2000;

// Heart Rate Variables
enum HRState { HR_IDLE, HR_STABILIZING, HR_MONITORING };
HRState hrState = HR_IDLE;
int currentBPM = 0;
unsigned long lastBeatTime = 0;
unsigned long stabilizeStartTime = 0;
int abnormalBeatCount = 0;
const unsigned long STABILIZE_DURATION = 18000; 
const unsigned long FINGER_REMOVED_TIMEOUT = 7000; 

// Multitasking Timers
unsigned long lastDisplayUpdate = 0;
unsigned long lastFirebaseSend = 0;
unsigned long lastFirebaseRead = 0;
const unsigned long DISPLAY_INTERVAL = 1000; 
const unsigned long FIREBASE_SEND_INTERVAL = 6000; 
const unsigned long FIREBASE_READ_INTERVAL = 10000; 

// ==========================================
// FUNCTION PROTOTYPES
// ==========================================
void connectToWiFi();
void setupFirebase();
void sendDataToFirebase();
void readPhoneNumberFromFirebase();
String getFormattedTime();
void triggerDanger(String cause);
void triggerSafe();
void sendSMS(String phoneNumber, String message);
void updateOLED();
void showBootScreen();

// ==========================================
// SETUP ROUTINE
// ==========================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- BOOTING SMART SAFETY DEVICE ---");
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);
  pinMode(GPS_LED_PIN, OUTPUT);
  
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(WIFI_LED_PIN, LOW);
  digitalWrite(GPS_LED_PIN, LOW);

  Serial.println("Initializing OLED Display...");
  Wire.begin();
  display.begin();
  showBootScreen();

  Serial.println("Starting GPS on UART2 (Baud 115200)...");
  GPS_SERIAL.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Serial.println("Starting GSM on UART1...");
  GSM_SERIAL.begin(115200, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  delay(3000); 
  GSM_SERIAL.println("AT+CMGF=1"); 
  Serial.println("GSM Ready.");

  Serial.println("Starting Pulse Sensor...");
  analogReadResolution(10); 
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);
  pulseSensor.setThreshold(THRESHOLD);
  
  if(pulseSensor.begin()){
    Serial.println("Pulse Sensor initialized successfully.");
  } else {
    Serial.println("WARNING: Pulse Sensor failed to initialize!");
  }

  // Connect IoT Modules
  connectToWiFi();
  timeClient.begin();
  timeClient.update();
  setupFirebase();
  
  Serial.println("--- SYSTEM READY ---");
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  unsigned long currentMillis = millis();
  
  // Maintain WiFi LED Status
  digitalWrite(WIFI_LED_PIN, (WiFi.status() == WL_CONNECTED) ? HIGH : LOW);

  // ------------------------------------------
  // 1. PROCESS GPS DATA 
  // ------------------------------------------
  if (GPS_SERIAL.available() > 0) {
    if (gps.encode(GPS_SERIAL.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        gpsValid = true;
        digitalWrite(GPS_LED_PIN, HIGH);
        locationURL = "https://maps.google.com/?q=" + String(latitude, 6) + "," + String(longitude, 6);
        
        if (!lastGpsValidState) {
          Serial.println("GPS: Lock Acquired! Lat: " + String(latitude, 5) + " Lng: " + String(longitude, 5));
          lastGpsValidState = true;
        }
      } else {
        gpsValid = false;
        digitalWrite(GPS_LED_PIN, LOW);
        locationURL = "";
        lastGpsValidState = false;
      }
    }
  }

  // ------------------------------------------
  // 2. PROCESS MANUAL BUTTON
  // ------------------------------------------
  int rawButtonState = digitalRead(BUTTON_PIN);
  if (rawButtonState != lastRawButtonState) lastDebounceTime = currentMillis;
  
  if ((currentMillis - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (rawButtonState != stableButtonState) {
      stableButtonState = rawButtonState;
      if (stableButtonState == LOW) { 
        buttonPressStart = currentMillis;
        buttonPressed = true;
        Serial.println("Button: Pressed down...");
      } else { 
        buttonPressed = false;
        unsigned long pressDuration = currentMillis - buttonPressStart;
        Serial.print("Button: Released. Press duration: "); Serial.print(pressDuration); Serial.println(" ms");
        
        if (pressDuration >= LONG_PRESS_TIME) {
          triggerSafe(); 
        } else if (pressDuration > 50) {
          if(!alertActive) triggerDanger("Manual Panic Button Pressed!"); 
        }
      }
    }
  }
  lastRawButtonState = rawButtonState;

  // ------------------------------------------
  // 3. PROCESS HEART RATE SENSOR
  // ------------------------------------------
  if (pulseSensor.sawStartOfBeat()) {
    currentBPM = pulseSensor.getBeatsPerMinute();
    lastBeatTime = currentMillis;
    
    Serial.print("♥ Beat detected! BPM: "); Serial.print(currentBPM);

    if (hrState == HR_IDLE) {
      hrState = HR_STABILIZING;
      stabilizeStartTime = currentMillis;
      Serial.println("[State: FINGER DETECTED -> STABILIZING]");
    } 
    else if (hrState == HR_STABILIZING) {
      Serial.println(" [State: READING... Please hold]");
      if (currentMillis - stabilizeStartTime > STABILIZE_DURATION) {
        hrState = HR_MONITORING;
        abnormalBeatCount = 0;
        Serial.println("\n*** HR Sensor Stabilized! Active Monitoring Started. ***");
      }
    } 
    else if (hrState == HR_MONITORING) {
      if (alertActive) {
         Serial.println("[State: DANGER MODE ACTIVE - Ignoring HR until SAFE reset]");
      } else {
         Serial.println(" [State: ACTIVE MONITORING]");
         if (currentBPM < 50 || currentBPM > 120) {
            abnormalBeatCount++;
            Serial.print("! ABNORMAL HR DETECTED ! (Count: "); Serial.print(abnormalBeatCount); Serial.println("/5)");
            
            if (abnormalBeatCount >= 5) {
              triggerDanger("Auto-Alert: Abnormal Heart Rate (" + String(currentBPM) + " BPM)!");
              abnormalBeatCount = 0; 
            }
         } else {
            abnormalBeatCount = 0; 
         }
      }
    }
  }

  if (hrState != HR_IDLE && (currentMillis - lastBeatTime > FINGER_REMOVED_TIMEOUT)) {
    hrState = HR_IDLE;
    currentBPM = 0;
    abnormalBeatCount = 0;
    Serial.println("\n[HR System] Finger removed. Returning to IDLE state.");
  }

  // ------------------------------------------
  // 4. PROCESS BUZZER ALERT
  // ------------------------------------------
  if (alertActive) {
    if (!inSilencePeriod) {
      if (currentMillis - lastBuzzerTime >= 200) { 
        buzzerState = !buzzerState;
        digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
        lastBuzzerTime = currentMillis;
        
        if (!buzzerState) { 
          beepCount++;
          if (beepCount >= 3) { 
            inSilencePeriod = true;
            beepCount = 0; 
          }
        }
      }
    } else {
      if (currentMillis - lastBuzzerTime >= 3000) {
        inSilencePeriod = false; 
        lastBuzzerTime = currentMillis;
      }
    }
  } else if (!buttonPressed) { 
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
    beepCount = 0;
    inSilencePeriod = false;
  }

  // ------------------------------------------
  // 5. IoT BACKGROUND TASKS
  // ------------------------------------------
  if (WiFi.status() == WL_CONNECTED) {
    if (Firebase.ready()) {
      if (currentMillis - lastFirebaseSend >= FIREBASE_SEND_INTERVAL) {
        sendDataToFirebase();
        lastFirebaseSend = currentMillis;
      }
      if (currentMillis - lastFirebaseRead >= FIREBASE_READ_INTERVAL) {
        readPhoneNumberFromFirebase();
        lastFirebaseRead = currentMillis;
      }
    }
    timeClient.update();
  }

  // ------------------------------------------
  // 6. UPDATE OLED DASHBOARD
  // ------------------------------------------
  if (currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    updateOLED();
    lastDisplayUpdate = currentMillis;
  }
}

// ==========================================
// IOT & HELPER FUNCTIONS
// ==========================================

void connectToWiFi() {
  Serial.println("WiFi: Attempting to connect to " + String(WIFI_SSID));
  display.firstPage();
  do {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(10, 30, "Connecting WiFi...");
    display.drawStr(10, 45, WIFI_SSID);
  } while (display.nextPage());

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    Serial.print(".");
    delay(300);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFI_LED_PIN, HIGH);
    Serial.println("\nWiFi: Connected! IP Address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi: Connection Failed. Running Offline Mode.");
  }
}

void setupFirebase() {
  Serial.println("Firebase: Initializing connection...");
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = LEGACY_TOKEN;
  config.timeout.networkReconnect = 10 * 1000;
  Firebase.setDoubleDigits(5);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true); 
  Serial.println("Firebase: Setup complete.");
}

void sendDataToFirebase() {
  FirebaseJson telemetryData;
  if (gpsValid) {
    telemetryData.set("Latitude", latitude);
    telemetryData.set("Longitude", longitude);
    telemetryData.set("LocationURL", locationURL);
  } else {
    telemetryData.set("LocationURL", "Unavailable");
  }
  
  telemetryData.set("BPM", currentBPM);
  telemetryData.set("Status", alertActive ? "DANGER" : "SAFE");
  
  if (Firebase.set(fbdo, F("/Telemetry"), telemetryData)) {
    Serial.println("Firebase: Telemetry updated successfully.");
  } else {
    Serial.println("Firebase Error (Upload): " + fbdo.errorReason());
  }
}

void readPhoneNumberFromFirebase() {
  if (Firebase.getString(fbdo, "/Phone_Number")) {
    String fetchedNumber = fbdo.stringData();
    if (fetchedNumber.length() > 5) { 
      if (recipientNumber != fetchedNumber) {
         Serial.println("Firebase: New Phone Number fetched: " + fetchedNumber);
         recipientNumber = fetchedNumber;
         
         // Format the number dynamically (+234 or 234 converted to 0)
         displayContactNumber = fetchedNumber;
         if (displayContactNumber.startsWith("+234")) {
           displayContactNumber = "0" + displayContactNumber.substring(4);
         } else if (displayContactNumber.startsWith("234")) {
           displayContactNumber = "0" + displayContactNumber.substring(3);
         }
         
         // Trigger the OLED non-blocking popup overlay
         showNewContactOverlay = true;
         newContactOverlayTime = millis();
      }
    }
  }
}

String getFormattedTime() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm *ti;
  ti = localtime(&rawTime);
  char buffer[25];
  snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d %02d:%02d:%02d", 
           ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900, 
           ti->tm_hour, ti->tm_min, ti->tm_sec);
  return String(buffer);
}

void triggerDanger(String cause) {
  Serial.println("\n==================================");
  Serial.println("EMERGENCY TRIGGERED: " + cause);
  Serial.println("==================================");
  
  alertActive = true;
  beepCount = 0;         
  inSilencePeriod = false; 
  currentTimestamp = getFormattedTime();
  
  String smsMsg = cause + "\n";
  String firebaseMsg = cause;

  if (gpsValid) {
    smsMsg += "Please help! Location: " + locationURL + "\n";
    firebaseMsg += " Location: " + locationURL;
  } else {
    smsMsg += "Please help! GPS unavailable.\n";
    firebaseMsg += " GPS unavailable.";
  }
  smsMsg += "Time: " + currentTimestamp;

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Firebase: Logging Danger Alert...");
    Firebase.setString(fbdo, "/Alert_Message", firebaseMsg + " | " + currentTimestamp);
  }
  
  sendSMS(recipientNumber, smsMsg);
}

void triggerSafe() {
  Serial.println("\n==================================");
  Serial.println("SAFE MODE TRIGGERED");
  Serial.println("==================================");
  
  alertActive = false;
  digitalWrite(BUZZER_PIN, LOW); 
  buzzerState = false;
  currentTimestamp = getFormattedTime();
  
  String smsMsg = "I am safe now. False alarm or danger passed.\n";
  String firebaseMsg = "Safe Mode Activated.";

  if (gpsValid) {
    smsMsg += "Current Location: " + locationURL + "\n";
    firebaseMsg += " Location: " + locationURL;
  }
  smsMsg += "Time: " + currentTimestamp;

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Firebase: Logging Safe Alert...");
    Firebase.setString(fbdo, "/Alert_Message", firebaseMsg + " | " + currentTimestamp);
  }
  
  sendSMS(recipientNumber, smsMsg);
}

void sendSMS(String phoneNumber, String message) {
  display.firstPage();
  do {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(10, 30, "SENDING SMS...");
    display.drawStr(10, 45, "Please Wait.");
  } while (display.nextPage());

  Serial.println("GSM: Initializing SMS sequence...");
  GSM_SERIAL.println("AT");
  delay(500);
  GSM_SERIAL.println("AT+CMGF=1");
  delay(500);
  GSM_SERIAL.println("AT+CMGS=\"" + phoneNumber + "\"");
  delay(500);
  GSM_SERIAL.print(message);
  delay(500);
  GSM_SERIAL.write(26); 
  delay(2000); 
  Serial.println("GSM: SMS Dispatched successfully.");
}

void updateOLED() {
  display.firstPage();
  do {
    display.setFont(u8g2_font_6x10_tf);
    
    // Check if we need to show the New Contact overlay
    if (showNewContactOverlay && (millis() - newContactOverlayTime < 3000)) {
       // Display Popup Overlay for 3 seconds
       display.drawStr(10, 15, "NEW CONTACT SYNCED");
       display.drawHLine(0, 22, 128); // draw line under text
       display.drawStr(10, 40, displayContactNumber.c_str());
       display.drawStr(10, 55, "System Updated.");
    } 
    else {
      // Normal Dashboard Mode
      showNewContactOverlay = false; // reset flag
      
      if (WiFi.status() == WL_CONNECTED) {
        display.drawStr(92, 10, "[WIFI]");
      } else {
        display.drawStr(92, 10, "[X-FI]"); 
      }

      if (alertActive) {
        display.drawStr(0, 10, "MODE: DANGER!");
      } else {
        display.drawStr(0, 10, "MODE: SAFE");
      }

      String hrStr = "HR: ";
      if (hrState == HR_IDLE) hrStr += "-- (Place Finger)";
      else if (hrState == HR_STABILIZING) hrStr += String(currentBPM) + " (Reading...)";
      else hrStr += String(currentBPM) + " BPM (Active)";
      display.drawStr(0, 24, hrStr.c_str());

      if (gpsValid) {
        display.drawStr(0, 38, "GPS: LOCKED");
        String latStr = "Lat: " + String(latitude, 5);
        String lngStr = "Lng: " + String(longitude, 5);
        display.drawStr(0, 50, latStr.c_str());
        display.drawStr(0, 62, lngStr.c_str());
      } else {
        display.drawStr(0, 38, "GPS: SEARCHING...");
        display.drawStr(0, 50, "Lat: --");
        display.drawStr(0, 62, "Lng: --");
      }
    }
  } while (display.nextPage());
}

void showBootScreen() {
  display.firstPage();
  do {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(10, 12, "SMART SAFETY");
    display.drawStr(25, 24, "DEVICE");
    
    display.drawHLine(0, 30, 128); 
    
    display.drawStr(0, 42, "UGWOKE CHIZARAM P.");
    display.drawStr(0, 54, "21CK029347");
  } while (display.nextPage());
  
  delay(3000); 
}