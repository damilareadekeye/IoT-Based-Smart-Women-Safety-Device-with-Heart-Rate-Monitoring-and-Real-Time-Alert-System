/*
 * ESP32 Heart Rate Monitor (Stripped & Optimized)
 * 
 * This sketch reads the DFRobot/PulseSensor PPG Heart Rate Sensor
 * on ESP32 Pin 36. It calculates the BPM and outputs it to the 
 * Serial Monitor. All unnecessary Wi-Fi and WebServer code has been removed.
 */

#include <PulseSensorPlayground.h>

// --- Configuration Variables ---
const int PULSE_INPUT = 36;  // Analog input pin (ESP32 GPIO 36 / VP)
const int PULSE_BLINK = 2;   // On-board LED pin for ESP32 (usually GPIO 2)
const int THRESHOLD = 550;   // Threshold to detect a beat.
                             // With 10-bit resolution, the idle signal is ~512,
                             // so 550 is a great starting point to filter noise.

// Create an instance of the PulseSensorPlayground object
PulseSensorPlayground pulseSensor;

void setup() {
  // Start Serial Monitor at 115200 baud
  Serial.begin(115200);

  // Give the Serial monitor a brief moment to catch up after resetting
  delay(1000);
  Serial.println("Starting Heart Rate Sensor...");

  // CRITICAL FOR ESP32 STABILITY:
  // The ESP32 defaults to 12-bit ADC (values from 0-4095).
  // The PulseSensor library's math and your threshold of 550 are designed
  // for a 10-bit ADC (values from 0-1023).
  // We force the ESP32 to 10-bit resolution here so it reads correctly.
  analogReadResolution(10);

  // --- Configure the PulseSensor Object ---
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.blinkOnPulse(PULSE_BLINK);  // Automatically blink the built-in LED on heartbeat
  pulseSensor.setThreshold(THRESHOLD);    // Set the threshold for beat detection

  // Initialize the sensor and check if it was successful
  if (pulseSensor.begin()) {
    Serial.println("PulseSensor Object successfully created and started!");
    Serial.println("Place your finger on the sensor...");
  } else {
    Serial.println("Failed to start PulseSensor. Check your wiring.");
  }
}

void loop() {
  // pulseSensor.sawStartOfBeat() constantly checks the analog pin.
  // It returns "true" the exact moment a beat is detected.
  if (pulseSensor.sawStartOfBeat()) {

    // Get the current calculated Beats Per Minute
    int myBPM = pulseSensor.getBeatsPerMinute();

    // Print the results to the Serial Monitor
    Serial.println("♥ A HeartBeat Happened ! ");
    Serial.print("BPM: ");
    Serial.println(myBPM);
  }

  // A small delay is best practice. It prevents the loop from running
  // unnecessarily fast and helps stabilize the timing of the library.
  delay(20);
}




// /*!
//  * @file  heartrateAnalogMode.h
//  * @brief  This is written for the heart rate sensor the company library. Mainly used for real
//  * @n  time measurement of blood oxygen saturation, based on measured values calculate heart rate values.
//  * @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
//  * @license  The MIT License (MIT)
//  * @author  [linfeng](Musk.lin@dfrobot.com)
//  * @maintainer  [qsjhyy](yihuan.huang@dfrobot.com)
//  * @version  V1.0
//  * @date  2022-04-26
//  * @url  https://github.com/DFRobot/DFRobot_Heartrate
//  */
// #include "DFRobot_Heartrate.h"
// #define heartratePin 36

// // DFRobot_Heartrate heartrate(DIGITAL_MODE);   // ANALOG_MODE or DIGITAL_MODE
// DFRobot_Heartrate heartrate(ANALOG_MODE);   // ANALOG_MODE or DIGITAL_MODE

// void setup() {
//   Serial.begin(115200);
// }

// void loop() {
//   uint8_t rateValue;
//   heartrate.getValue(heartratePin);   // A1 foot sampled values
//   rateValue = heartrate.getRate();   // Get heart rate value
//   if(rateValue)  {
//     Serial.println(rateValue);
//   }
//   delay(20);
// }


// /*
//  * ESP32 DFRobot Heart Rate Monitor
//  *
//  * Uses the official DFRobot algorithm to filter out noise
//  * and wait for a stable pulse before displaying the BPM.
//  */

// #include "DFRobot_Heartrate.h"

// // --- Configuration Variables ---
// const int heartratePin = 36; // ESP32 Analog Pin (VP)

// // Initialize the DFRobot Heartrate object in ANALOG_MODE
// DFRobot_Heartrate heartrate(ANALOG_MODE);

// void setup() {
//   Serial.begin(115200);
//   delay(1000); // Let Serial monitor catch up

//   Serial.println("DFRobot Heart Rate Sensor Initialized.");
//   Serial.println("Place your finger gently on the sensor and STAY STILL...");
// }

// void loop() {
//   uint8_t rateValue;

//   // 1. Sample the analog pin
//   heartrate.getValue(heartratePin);

//   // 2. Ask the library to calculate the heart rate
//   rateValue = heartrate.getRate();

//   // 3. The library returns 0 if the data is noisy or stabilizing.
//   //    It only returns a number > 0 when it has a confident heartbeat lock.
//   if(rateValue > 0)  {
//     Serial.print("♥ Stable Heart Rate: ");
//     Serial.print(rateValue);
//     Serial.println(" BPM");
//   }

//   // Delay 20ms to allow the library to sample at 50Hz (Standard for this sensor)
//   delay(20);
// }