#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "display.h"
#include "screen_slider.h"
#include "usage_display.h"
#include "http_client.h"

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define ROBOEYES_FPS 30
#define DISPLAY_TICK_MS 1

QueueHandle_t displayQueue;
QueueHandle_t httpQueue;

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
static RoboEyes<Adafruit_SSD1306> eyes(oled);
ScreenSlider slider(&oled, &eyes);

static void init_hardware() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] init failed");
    vTaskDelete(nullptr);
  }
}

static void init_eyes() {
  eyes.begin(OLED_WIDTH, OLED_HEIGHT, ROBOEYES_FPS);
  eyes.setAutoblinker(ON, 4, 0);
  eyes.setWidth(40, 40);
  eyes.setHeight(40, 40);
  eyes.setBorderradius(8, 14);
  eyes.setMood(ANGRY);
}

static void handle_button_press() {
  slider.next_screen();
  bool trigger = true;
  xQueueSend(httpQueue, &trigger, 0);
}

static void httpTask(void *parameters) {
  httpQueue = xQueueCreate(10, sizeof(bool));
  bool dummy;

  for (;;) {
    if (xQueueReceive(httpQueue, &dummy, portMAX_DELAY) == pdTRUE) {
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.setTimeout(REQUEST_TIMEOUT_MS);
        http.begin(SERVER_HOST, SERVER_PORT, SERVER_PATH);
        int code = http.GET();
        
        if (code == 200) {
          String payload = http.getString();
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, payload);
          if (!error) {
            const char *weekly = doc["usage"]["weekly"]["formatted"];
            const char *pace = doc["pace"]["weekly"];
            if (weekly && pace) {
              slider.set_usage_data(weekly, pace);
            }
          }
        }
        http.end();
      }
    }
  }
}

void set_usage_data(const char *weekly, const char *pace) {
  slider.set_usage_data(weekly, pace);
}

static void displayTask(void *parameters) {
  init_hardware();
  init_eyes();

  displayQueue = xQueueCreate(10, sizeof(bool));
  bool button_pressed = false;

  xTaskCreatePinnedToCore(
      httpTask,
      "HTTP task",
      5000,
      nullptr,
      1,
      nullptr,
      CONFIG_ARDUINO_RUNNING_CORE);

  for (;;) {
    if (xQueueReceive(displayQueue, &button_pressed, 0) == pdTRUE && button_pressed) {
      handle_button_press();
    }

    slider.render();
    vTaskDelay(DISPLAY_TICK_MS / portTICK_PERIOD_MS);
  }
}

void startDisplayTask() {
  xTaskCreatePinnedToCore(
      displayTask,
      "Display task",
      6144,
      nullptr,
      1,
      nullptr,
      CONFIG_ARDUINO_RUNNING_CORE);
}
