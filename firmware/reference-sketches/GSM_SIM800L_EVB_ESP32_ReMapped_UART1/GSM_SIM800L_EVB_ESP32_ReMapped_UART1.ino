#include <Arduino.h>
#include <HardwareSerial.h>

// GSM Module (SIM800L) Pin Definitions for remapped UART1 (ReMAPPED from UART 1).
#define GSM_RX_PIN 25  // SIM800L RX (ESP32 TX)      (do not use pin 4 and 5 for remapping of UART 1)
#define GSM_TX_PIN 33  // SIM800L TX (ESP32 RX)      (do not use pin 4 and 5 for remapping of UART 1)
#define GSM_SERIAL Serial1

String sender_NO = "+2348163180829";  // Your recipient number

void setup() {
  // Start debugging serial on UART0
  Serial.begin(115200);
  Serial.println("ESP32 Starting...");

  // Initialize GSM on UART1 with remapped pins
  GSM_SERIAL.begin(115200, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);  // Match Nano baud rate
  Serial.println("Waiting for SIM800L to power up...");
  Serial.println("Initializing...");
  delay(5000);  // 5s delay (worked on Nano with 1s, but giving extra time)
  while (!GSM_SERIAL.available()) {
    GSM_SERIAL.println("AT");
    delay(1000);
    Serial.println("Connecting...");
  }
  GSM_SERIAL.println("AT+CMGF=1");  //Set SMS to Text Mode
  delay(1000);
  Serial.println("Connected!");
  delay(1000);
  msgBody();  // Send SMS as in your Nano code
}

void loop() {
  // updateSerial();  // Continuously echo responses for debugging
}

// updateSerial() is not necessary unless you need to manually send AT commands from the Serial Monitor to the SIM800L for debugging.
// If you're only sending predefined SMS messages in code, you can remove it without issues. 🚀
// void updateSerial() {
//   delay(500);
//   while (Serial.available()) {
//     GSM_SERIAL.write(Serial.read());  // Forward Serial to GSM
//   }
//   while (GSM_SERIAL.available()) {
//     Serial.write(GSM_SERIAL.read());  // Forward GSM to Serial
//   }
// }

void msgBody() {
  Serial.println("Sending AT...");
  GSM_SERIAL.println("AT");  //Once the handshake test is successful, it will back to OK
  // updateSerial();  //this isnt necessary if the string on Serial isnt needed to be writen to GSM Module.
  delay(500);
  Serial.println("Setting text mode...");
  GSM_SERIAL.println("AT+CMGF=1");  // Configuring TEXT mode
                                  // updateSerial();
  delay(500);
  Serial.println("Preparing SMS...");
  GSM_SERIAL.println("AT+CMGS=\"" + sender_NO + "\"");  //Mobile phone number to send message
                                                      // updateSerial();
  delay(500);
  String SMS;
  SMS = "";
  //  SMS = "Hello,Time: " + hour + ":" + minute + "\rDate:" + day + "/" + month + "/" + year;
  SMS = "Deewan is a good coder!!!";
  Serial.println("Sending message: " + SMS);
  GSM_SERIAL.print(SMS);  //text content
                        // GSM_SERIAL.print("Last Minute Engineers | lastminuteengineers.com");  //text content
                        // updateSerial();
  delay(500);
  GSM_SERIAL.write(26);
  delay(1000);  // Give time for response
  Serial.println("Message Sent.");
}
