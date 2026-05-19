#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <Wire.h>
#include "env_config.h"
#include "display.h"

#ifndef OLED_SDA
#define OLED_SDA 21
#endif

#ifndef OLED_SCL
#define OLED_SCL 22
#endif

#ifndef OLED_ADDR
#define OLED_ADDR 0x3C
#endif

#ifndef OLED_WIDTH
#define OLED_WIDTH 128
#endif

#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif

#ifndef ROBOEYES_FPS
#define ROBOEYES_FPS 30
#endif

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
static RoboEyes<Adafruit_SSD1306> eyes(oled);

static void displayTask(void *parameters) {
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] init failed");
    vTaskDelete(nullptr);
    return;
  }

  eyes.begin(OLED_WIDTH, OLED_HEIGHT, ROBOEYES_FPS);
  eyes.setAutoblinker(ON, 3, 2);
  eyes.setIdleMode(ON, 2, 2);
  eyes.setMood(DEFAULT);

  for (;;) {
    eyes.update();
    vTaskDelay(1 / portTICK_PERIOD_MS);
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
