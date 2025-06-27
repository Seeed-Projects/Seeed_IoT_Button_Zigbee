#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "ZigbeeMultiButton.h"

/* Button Configuration */
#define BUTTON_PIN 9
#define MULTI_CLICK_TIME 400       // Maximum time between clicks for multi-click (ms)
#define SHORT_LONG_PRESS_TIME 1000 // Minimum time for short long press (1 second)
#define LONG_PRESS_TIME 5000       // Minimum time for long press (5 seconds)
#define DEBOUNCE_TIME 50           // Debounce time (ms)

/* LED Configuration */
#define BLUE_LED_PIN 2
#define RGB_ENABLE_PIN 18
#define RGB_PIN 19 // GPIO19 for WS2812 LED
#define NUM_RGBS 1 // Single LED
CRGB rgbs[NUM_RGBS];

/* Button States */
typedef enum
{
  BUTTON_IDLE,
  BUTTON_PRESSED,
  BUTTON_RELEASED,
  BUTTON_MULTI_CLICK_WAIT,
} ButtonState;

/* Button Events */
typedef enum
{
  BUTTON_EVENT_SINGLE_CLICK = 1,
  BUTTON_EVENT_DOUBLE_CLICK = 2,
  BUTTON_EVENT_SHORT_LONG_PRESS = 3,
  BUTTON_EVENT_TRIPLE_CLICK = 4,
  BUTTON_EVENT_LONG_PRESS = 5,
} ButtonEvent;

/* Zigbee Configuration */
#define BUTTON_ENDPOINT 10
ZigbeeMultiButton zbIoTButton = ZigbeeMultiButton(BUTTON_ENDPOINT);

/* Global Variables */
ButtonState buttonState = BUTTON_IDLE;
unsigned long pressStartTime = 0;
unsigned long lastReleaseTime = 0;
int clickCount = 0;
bool longPressHandled = false;
QueueHandle_t eventQueue;

/********************* LED Functions **************************/
void ledBlink()
{
  uint8_t rand = random8();
  for (int i = 0; i < 2; i++)
  {
    rgbs[0] = CHSV(rand, 255, 255); // Random color
    FastLED.show();
    vTaskDelay(200 / portTICK_PERIOD_MS);
    rgbs[0] = CRGB::Black;
    FastLED.show();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void ledBreathing()
{
  uint8_t hue = random8(); // Random color hue
  for (int i = 0; i < 1; i++)
  { // one breathing cycles
    // Brighten
    for (int brightness = 0; brightness <= 255; brightness += 5)
    {
      rgbs[0] = CHSV(hue, 255, brightness);
      FastLED.show();
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    // Dim
    for (int brightness = 255; brightness >= 0; brightness -= 5)
    {
      rgbs[0] = CHSV(hue, 255, brightness);
      FastLED.show();
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
  rgbs[0] = CRGB::Black;
  FastLED.show();
}

void ledRainbow()
{
  for (int hue = 0; hue < 128; hue += 10)
  {
    rgbs[0] = CHSV(hue, 255, 255);
    FastLED.show();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  rgbs[0] = CRGB::Black;
  FastLED.show();
}

/********************* Button Functions **************************/
void handleButtonEvent(ButtonEvent event)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(eventQueue, &event, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}

void processButton()
{
  static unsigned long lastDebounceTime = 0;
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  unsigned long currentTime = millis();

  // Debounce check
  if (currentTime - lastDebounceTime < DEBOUNCE_TIME)
  {
    return;
  }
  lastDebounceTime = currentTime;

  switch (buttonState)
  {
  case BUTTON_IDLE:
    if (buttonPressed)
    {
      pressStartTime = currentTime;
      buttonState = BUTTON_PRESSED;
      longPressHandled = false;
      clickCount = 0; // Reset click count
    }
    break;

  case BUTTON_PRESSED:
    if (!buttonPressed)
    {
      // Button released
      unsigned long pressDuration = currentTime - pressStartTime;
      if (pressDuration < SHORT_LONG_PRESS_TIME)
      {
        // Short press, consider as a click
        clickCount++;
        buttonState = BUTTON_MULTI_CLICK_WAIT;
        lastReleaseTime = currentTime;
      }
      else if (pressDuration < LONG_PRESS_TIME)
      {
        // Short long press (1s <= duration < 5s)
        handleButtonEvent(BUTTON_EVENT_SHORT_LONG_PRESS);
        buttonState = BUTTON_IDLE;
      }
      else
      {
        // Long press handled already
        buttonState = BUTTON_IDLE;
      }
    }
    else if (!longPressHandled && (currentTime - pressStartTime >= LONG_PRESS_TIME))
    {
      // Long press detected (5 seconds)
      handleButtonEvent(BUTTON_EVENT_LONG_PRESS);
      longPressHandled = true;
    }
    break;

  case BUTTON_MULTI_CLICK_WAIT:
    if (buttonPressed)
    {
      // Another press within multi-click time
      pressStartTime = currentTime;
      buttonState = BUTTON_PRESSED;
    }
    else if (currentTime - lastReleaseTime >= MULTI_CLICK_TIME)
    {
      // No more clicks, handle the event
      if (clickCount == 1)
      {
        handleButtonEvent(BUTTON_EVENT_SINGLE_CLICK);
      }
      else if (clickCount == 2)
      {
        handleButtonEvent(BUTTON_EVENT_DOUBLE_CLICK);
      }
      else if (clickCount >= 3)
      {
        handleButtonEvent(BUTTON_EVENT_TRIPLE_CLICK);
      }
      buttonState = BUTTON_IDLE;
    }
    break;
  }
}

/********************* FreeRTOS Tasks **************************/
void buttonTask(void *pvParameters)
{
  while (1)
  {
    processButton();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void mainTask(void *pvParameters)
{
  ButtonEvent event;
  while (1)
  {
    if (xQueueReceive(eventQueue, &event, portMAX_DELAY) == pdTRUE)
    {
      switch (event)
      {
      case BUTTON_EVENT_SINGLE_CLICK:
        Serial.println("Single Click: Blink LED");
        ledBlink();
        if (Zigbee.connected())
        {
          zbIoTButton.click(1);
        }
        break;
      case BUTTON_EVENT_DOUBLE_CLICK:
        Serial.println("Double Click: Breathing LED");
        ledBreathing();
        if (Zigbee.connected())
        {
          zbIoTButton.click(2);
        }
        break;
      case BUTTON_EVENT_SHORT_LONG_PRESS:
        Serial.println("Short Long Press: Rainbow LED");
        ledRainbow();
        if (Zigbee.connected())
        {
          zbIoTButton.click(3);
        }
        break;
      case BUTTON_EVENT_LONG_PRESS:
        Serial.println("Long Press: Reset Zigbee");
        delay(1000);
        Zigbee.factoryReset();
        break;
      case BUTTON_EVENT_TRIPLE_CLICK:
        Serial.println("Triple Click: Ignored");
        break;
      }
    }
  }
}

void blueLedTask(void *pvParameters)
{
  pinMode(BLUE_LED_PIN, OUTPUT);
  while (1)
  {
    if (!Zigbee.connected())
    {
      digitalWrite(BLUE_LED_PIN, LOW);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      digitalWrite(BLUE_LED_PIN, HIGH);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    else
    {
      digitalWrite(BLUE_LED_PIN, LOW);
    }
  }
}
/********************* Zigbee Functions **************************/
void setupZigbee()
{
  // Set Zigbee device information
  zbIoTButton.setManufacturerAndModel("Seeed Studio", "IoT_Button");

  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbIoTButton);

  Serial.println("Starting Zigbee...");
  if (!Zigbee.begin())
  {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }
  else
  {
    Serial.println("Zigbee started successfully!");
  }

}

/********************* Arduino Setup **************************/
void setup()
{
  Serial.begin(115200);

  Serial.println("Zigbee IoT Button Starting...");

  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RGB_ENABLE_PIN, OUTPUT);
  digitalWrite(RGB_ENABLE_PIN, HIGH);

  // Initialize LED
  FastLED.addLeds<WS2812, RGB_PIN, GRB>(rgbs, NUM_RGBS);
  FastLED.setBrightness(50);
  ledRainbow();

  // Initialize Zigbee
  setupZigbee();

  // Create event queue
  eventQueue = xQueueCreate(10, sizeof(ButtonEvent));
  if (eventQueue == NULL)
  {
    Serial.println("Failed to create event queue!");
    ESP.restart();
  }

  // Create FreeRTOS tasks
  xTaskCreate(buttonTask, "ButtonTask", 2048, NULL, 2, NULL);
  xTaskCreate(blueLedTask, "BlueLedTask", 1024, NULL, 1, NULL);
  xTaskCreate(mainTask, "MainTask", 2048, NULL, 1, NULL);
}

/********************* Arduino Loop **************************/
void loop()
{
  if (!Zigbee.connected())
  {
    Serial.print(".");
    delay(100);
  }
  else
  {
    delay(1000);
  }
}