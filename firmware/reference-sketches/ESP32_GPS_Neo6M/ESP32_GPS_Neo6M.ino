#include <TinyGPS++.h>

#define RXD2 16
#define TXD2 17
#define gpsIndicator 4  // Indicator for valid GPS data

TinyGPSPlus gps;
double latitude, longitude;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  pinMode(gpsIndicator, OUTPUT);
}

void loop() {
  if (Serial2.available() > 0) {
    Serial.println(".");  //this is to see if the TX and RX is connected properly.
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();

        Serial.print("Latitude: ");
        Serial.println(latitude);
        Serial.print("Longitude: ");
        Serial.println(longitude);

        digitalWrite(gpsIndicator, HIGH);  // Location is valid
      } else {
        latitude = 0.0;
        longitude = 0.0;
        Serial.println("Invalid GPS location.");
        digitalWrite(gpsIndicator, LOW);  // Invalid location
      }
    }
  }
}


// #include <TinyGPS++.h>
// #include <SoftwareSerial.h>

// // Arduino Nano Pins:
// // Pin 2 (RX) connects to GPS TX
// // Pin 3 (TX) connects to GPS RX
// const int RXPin = 2;
// const int TXPin = 3;
// const int gpsIndicator = 4; // Using built-in LED (Pin 13) or Pin 33 (if wired)
// const uint32_t GPSBaud = 9600;

// // Objects
// TinyGPSPlus gps;
// SoftwareSerial ss(RXPin, TXPin);

// unsigned long lastTimeDataReceived = 0;

// void setup() {
//   Serial.begin(9600);
//   ss.begin(115200);

//   pinMode(gpsIndicator, OUTPUT);

//   Serial.println(F("GPS Module Test Starting..."));
//   Serial.println(F("Checking for GPS data on Pins 2 and 3..."));
// }

// void loop() {
//   // Read data from GPS
//   while (ss.available() > 0) {
//     char c = ss.read();
//     lastTimeDataReceived = millis(); // Reset timer because we are receiving bytes

//     // Optional: Uncomment the line below to see raw NMEA data for debugging
//     Serial.print(c);

//     if (gps.encode(c)) {
//       displayInfo();
//     }
//   }

//   // DIAGNOSTIC: Check if we have received ANY data in the last 5 seconds
//   if (millis() > 5000 && (millis() - lastTimeDataReceived) > 5000) {
//     Serial.println(F("ERROR: No data received from GPS. Check wiring!"));
//     Serial.println(F("Try swapping the TX and RX pins on the GPS module."));
//     digitalWrite(gpsIndicator, LOW);
//     delay(2000);
//   }
// }

// void displayInfo() {
//   if (gps.location.isValid()) {
//     digitalWrite(gpsIndicator, HIGH); // GPS Fix acquired

//     Serial.print(F("Latitude: "));
//     Serial.println(gps.location.lat(), 6);
//     Serial.print(F("Longitude: "));
//     Serial.println(gps.location.lng(), 6);
//     Serial.print(F("Satellites: "));
//     Serial.println(gps.satellites.value());
//   } else {
//     digitalWrite(gpsIndicator, LOW);
//     Serial.print(F("Searching for satellites... Characters processed: "));
//     Serial.println(gps.charsProcessed());
//   }
// }