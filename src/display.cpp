#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "display.h"
#include "screen_slider.h"
#include "usage_display.h"

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define ROBOEYES_FPS 30

QueueHandle_t displayQueue;
QueueHandle_t httpQueue;
SemaphoreHandle_t sliderMutex;

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
  if (xSemaphoreTake(sliderMutex, pdMS_TO_TICKS(100))) {
    slider.next_screen();
    xSemaphoreGive(sliderMutex);
  }
}

void set_usage_data(const char *weekly, const char *pace, uint8_t percent) {
  if (xSemaphoreTake(sliderMutex, pdMS_TO_TICKS(100))) {
    slider.set_usage_data(weekly, pace, percent);
    xSemaphoreGive(sliderMutex);
  }
}

static void displayTask(void *parameters) {
  init_hardware();
  init_eyes();

  bool button_pressed = false;
  TickType_t frameDelay = pdMS_TO_TICKS(1000 / ROBOEYES_FPS);

  for (;;) {
    if (xQueueReceive(displayQueue, &button_pressed, 0) == pdTRUE && button_pressed) {
      handle_button_press();
    }

    if (xSemaphoreTake(sliderMutex, pdMS_TO_TICKS(100))) {
      slider.render();
      xSemaphoreGive(sliderMutex);
    }
    vTaskDelay(frameDelay);
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
      1);  
}
