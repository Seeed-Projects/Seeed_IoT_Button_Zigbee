#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

/* Button Configuration */
#define BUTTON_PIN 9
#define MULTI_CLICK_TIME 400    // Maximum time between clicks for multi-click (ms)
#define SHORT_LONG_PRESS_TIME 1000 // Minimum time for short long press (1 second)
#define LONG_PRESS_TIME 5000    // Minimum time for long press (5 seconds)
#define DEBOUNCE_TIME 50         // Debounce time (ms)

/* Zigbee Configuration */
#define BUTTON_ENDPOINT_NUMBER 10

/* Button States */
typedef enum {
  BUTTON_IDLE,
  BUTTON_PRESSED,
  BUTTON_RELEASED,
  BUTTON_MULTI_CLICK_WAIT,
} ButtonState;

/* Button Events */
typedef enum {
  BUTTON_EVENT_SINGLE_CLICK = 1,
  BUTTON_EVENT_DOUBLE_CLICK = 2,
  BUTTON_EVENT_SHORT_LONG_PRESS = 3,
  BUTTON_EVENT_TRIPLE_CLICK = 4,
  BUTTON_EVENT_LONG_PRESS = 5,
} ButtonEvent;

/* Global Variables */
ZigbeeMultiButton zbIoTButton = ZigbeeMultiButton(BUTTON_ENDPOINT_NUMBER);
ButtonState buttonState = BUTTON_IDLE;
unsigned long pressStartTime = 0;
unsigned long lastReleaseTime = 0;
int clickCount = 0;
bool longPressHandled = false;

/********************* Button Functions **************************/
void handleButtonEvent(ButtonEvent event) {
  // Report the button event via Zigbee
  switch (event) {
    case BUTTON_EVENT_SINGLE_CLICK:
      Serial.println("Single Click Detected");
      zbIoTButton.click(1);
      break;
    case BUTTON_EVENT_DOUBLE_CLICK:
      Serial.println("Double Click Detected");
      zbIoTButton.click(2);
      break;
    case BUTTON_EVENT_SHORT_LONG_PRESS:
      Serial.println("Short Long Press Detected (<5s)");
      zbIoTButton.click(3);
      break;
    case BUTTON_EVENT_TRIPLE_CLICK:
      Serial.println("Triple Click Detected");
      zbIoTButton.click(4);
      break;
    case BUTTON_EVENT_LONG_PRESS:
      Serial.println("Long Press Detected (5s+)");
      Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
      delay(1000);
      Zigbee.factoryReset();
      break;
  }
}

void processButton() {
  static unsigned long lastDebounceTime = 0;
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  unsigned long currentTime = millis();

  // Debounce check
  if (currentTime - lastDebounceTime < DEBOUNCE_TIME) {
    return;
  }
  lastDebounceTime = currentTime;

  switch (buttonState) {
    case BUTTON_IDLE:
      if (buttonPressed) {
        pressStartTime = currentTime;
        buttonState = BUTTON_PRESSED;
        longPressHandled = false;
        clickCount = 0; // Reset click count
      }
      break;

    case BUTTON_PRESSED:
      if (!buttonPressed) {
        // Button released
        unsigned long pressDuration = currentTime - pressStartTime;
        if (pressDuration < SHORT_LONG_PRESS_TIME) {
          // Short press, consider as a click
          clickCount++;
          buttonState = BUTTON_MULTI_CLICK_WAIT;
          lastReleaseTime = currentTime;
        } else if (pressDuration < LONG_PRESS_TIME) {
          // Short long press (1s <= duration < 5s)
          handleButtonEvent(BUTTON_EVENT_SHORT_LONG_PRESS);
          buttonState = BUTTON_IDLE;
        } else {
          // Long press handled already
          buttonState = BUTTON_IDLE;
        }
      } else if (!longPressHandled && (currentTime - pressStartTime >= LONG_PRESS_TIME)) {
        // Long press detected (5 seconds)
        handleButtonEvent(BUTTON_EVENT_LONG_PRESS);
        longPressHandled = true;
      }
      break;

    case BUTTON_MULTI_CLICK_WAIT:
      if (buttonPressed) {
        // Another press within multi-click time
        pressStartTime = currentTime;
        buttonState = BUTTON_PRESSED;
      } else if (currentTime - lastReleaseTime >= MULTI_CLICK_TIME) {
        // No more clicks, handle the event
        if (clickCount == 1) {
          handleButtonEvent(BUTTON_EVENT_SINGLE_CLICK);
        } else if (clickCount == 2) {
          handleButtonEvent(BUTTON_EVENT_DOUBLE_CLICK);
        } else if (clickCount >= 3) {
          handleButtonEvent(BUTTON_EVENT_TRIPLE_CLICK);
        }
        buttonState = BUTTON_IDLE;
      }
      break;
  }
}

/********************* Arduino Setup **************************/
void setup() {
  Serial.begin(115200);
  
  Serial.println("Zigbee IoT Button Starting...");

  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Set Zigbee device information
  zbIoTButton.setManufacturerAndModel("Seeed Studio", "IoT_Button");

  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbIoTButton);

  Serial.println("Starting Zigbee...");
  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin()) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  } else {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

/********************* Arduino Loop **************************/
void loop() {
  // Process button state
  processButton();

  delay(100);
}