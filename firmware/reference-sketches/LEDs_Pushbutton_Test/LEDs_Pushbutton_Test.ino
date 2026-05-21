/*
 * Button State Tracking (Pressed & Released)
 * Button connected to GPIO 5 and GND
 */

const int buttonPin = 5;

// Variables to track the button state
int currentButtonState = HIGH;  // Current stable state of the button
int lastButtonState = HIGH;     // Previous reading from the button

// Debounce variables
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  // Start the serial monitor to see the results
  Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(18, OUTPUT);               // buzzer
  pinMode(19, OUTPUT);               // Green WiFi LED
  pinMode(4, OUTPUT);                // Yellow GPS LED.
  pinMode(buttonPin, INPUT_PULLUP);  // Pushbutton

  // pinMode(32, OUTPUT);  // blue led - connection established.
  // Initialize the button pin with the internal pull-up resistor.
  // This means the pin reads HIGH when unpressed, and LOW when pressed.
  delay(3000);  // wait for a second

  Serial.println("Button Tracker Initialized. Waiting for press...");
}

void loop() {
  // Read the raw state of the switch into a local variable:
  int reading = digitalRead(buttonPin);
  Serial.println("Reading:");
  Serial.println(reading);

  // Check if the button state has changed (due to pressing, releasing, or noise)
  if (reading != lastButtonState) {
    // Reset the debouncing timer
    lastDebounceTime = millis();
  }

  // If the state has been stable longer than the debounce delay, we register it
  if ((millis() - lastDebounceTime) > debounceDelay) {

    // If the button state has actually changed from our last stable state
    if (reading != currentButtonState) {
      currentButtonState = reading;

      // Because we use INPUT_PULLUP:
      // LOW means the button is PRESSED (connected to ground)
      // HIGH means the button is RELEASED
      if (currentButtonState == LOW) {
        Serial.println("Button is PRESSED!");
      } else {
        Serial.println("Button is RELEASED!");
      }
    }
  }

  // Save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;

  // digitalWrite(18, HIGH);  // Buzzer.
  // digitalWrite(19, HIGH);  // Green WiFi LED.
  digitalWrite(4, HIGH);  // Yellow GPS LED.

  delay(1000);  // wait for a second
  // digitalWrite(18, LOW);   // Buzzer.
  // digitalWrite(19, LOW);   // Green WiFi LED.
  digitalWrite(4, LOW);  // Yellow GPS LED.

  delay(1000);  // wait for a second
}