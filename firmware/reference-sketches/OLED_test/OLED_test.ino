/* 
 * 1.3" OLED Display Test (I2C)
 * SDA = A4, SCL = A5 
 * Library required: U8g2 (by oliver)
 */

#include <Wire.h>
#include <U8g2lib.h>

// Initialize the OLED for SH1106 (Standard for 1.3" I2C OLEDs).
// If your display looks glitchy or has random dots on the edge, 
// swap this to: U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// An array of random statements to display
const char* randomStatements[] = {
  "Arduino is awesome!",
  "Keep on coding...",
  "Having a good day?",
  "I love technology.",
  "Look at me glow!",
  "System Nominal."
};

// Function to draw text on the screen
void showOLED(const char* line1, const char* line2) {
  oled.firstPage();
  do {
    // Draw the top header
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(0, 12, "MY OLED SCREEN");
    oled.drawHLine(0, 15, 128); // Draw a horizontal line under the header

    // Draw Line 1 (Large Text)
    oled.setFont(u8g2_font_10x20_tf);
    oled.drawStr(10, 40, line1);

    // Draw Line 2 (Small Text)
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(10, 60, line2);
    
  } while (oled.nextPage());
}

void setup() {
  Wire.begin();
  oled.begin();

  // Create a random seed by reading an empty analog pin 
  // so the randomness is different every time you turn it on
  randomSeed(analogRead(A0));

  // Show a quick boot-up screen
  showOLED("Booting...", "Please wait");
  delay(2000); 
}

void loop() {
  // Generate a random number between 0 and 5 (since we have 6 phrases)
  int randomIndex = random(0, 6); 

  // Display "Hello World" along with the random phrase chosen
  showOLED("Hello World!", randomStatements[randomIndex]);

  // Wait 3 seconds before picking a new random statement
  delay(3000);
}