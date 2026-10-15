#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"
#include "ButtonInput.h"

/* Button Configuration */
#define BUTTON_PIN 9

/*
 * Timing knobs (milliseconds). Change these values, rebuild, and flash
 * to match how you press the button.
 * 时间参数（毫秒）。按自己的手感改这些值，重新编译烧录即可。
 *
 * DEBOUNCE_TIME:         pin must stay unchanged this long before an edge counts
 * MIN_PRESS_TIME:        shortest stable press counted as a click
 * DOUBLE_CLICK_TIME:     max gap from first release to second press
 * LONG_PRESS_TIME:       hold duration that starts a long-press / factory reset
 * BUTTON_POLL_INTERVAL:  how often loop() samples the pin
 */
#define DEBOUNCE_TIME 20
#define MIN_PRESS_TIME 30
#define DOUBLE_CLICK_TIME 600
#define LONG_PRESS_TIME 1000
#define BUTTON_POLL_INTERVAL 10

/* Zigbee Configuration */
#define BUTTON_ENDPOINT_NUMBER 10

/* Global Variables */
ZigbeeMultiButton zbIoTButton = ZigbeeMultiButton(BUTTON_ENDPOINT_NUMBER);
ButtonInput buttonInput({DEBOUNCE_TIME, MIN_PRESS_TIME, DOUBLE_CLICK_TIME, LONG_PRESS_TIME});

/********************* Button Functions **************************/
void handleButtonEvent(ButtonEvent event) {
  // Report the button event via Zigbee
  switch (event) {
    case BUTTON_EVENT_SINGLE_CLICK:
      Serial.println("Single Click Detected");
      zbIoTButton.click1();
      break;
    case BUTTON_EVENT_DOUBLE_CLICK:
      Serial.println("Double Click Detected");
      zbIoTButton.click2();
      break;
    case BUTTON_EVENT_LONG_PRESS:
      Serial.println("Long Press Detected");
      Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
      delay(1000);
      Zigbee.factoryReset();
      break;
    case BUTTON_EVENT_NONE:
      break;
  }
}

/********************* Arduino Setup **************************/
void setup() {
  Serial.begin(115200);
  
  Serial.println("Zigbee IoT Button Starting...");
  Serial.print("Button timing (ms): debounce=");
  Serial.print(DEBOUNCE_TIME);
  Serial.print(" min_press=");
  Serial.print(MIN_PRESS_TIME);
  Serial.print(" double_click=");
  Serial.print(DOUBLE_CLICK_TIME);
  Serial.print(" long_press=");
  Serial.print(LONG_PRESS_TIME);
  Serial.print(" poll=");
  Serial.println(BUTTON_POLL_INTERVAL);

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
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  ButtonEvent event = buttonInput.update(pressed, millis());
  if (event != BUTTON_EVENT_NONE) {
    handleButtonEvent(event);
  }

  delay(BUTTON_POLL_INTERVAL);
}
