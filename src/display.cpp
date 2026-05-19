#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "env_config.h"
#include "display.h"

#ifndef OLED_ADDR
#define OLED_ADDR 0x3C
#endif

#ifndef OLED_WIDTH
#define OLED_WIDTH 128
#endif

#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

static void displayTask(void *parameters) {

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] init failed");
    vTaskDelete(nullptr);
    return;
  }

  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 20);
  oled.println("Hello");
  oled.println("World!");
  oled.display();

  for (;;) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
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
