#pragma once
#include <stdint.h>
#include <Adafruit_SSD1306.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct Coords {
  uint8_t x;
  uint8_t y;
};

class OLED {
public:
  virtual void display_text(const char *text, Coords coords = {0, 0}, uint8_t size = 3, bool reset = true);
};

class SSD1306OLED : public OLED {
private:
  Adafruit_SSD1306 *ssd1306;
public:
  SSD1306OLED(Adafruit_SSD1306 *ssd1306)
    : ssd1306(ssd1306) {}
    

  void display_text(const char *text, Coords coords = {0, 0}, uint8_t size = 3, bool reset = true) {
    if (reset) {
      this->ssd1306->clearDisplay();
    }
    this->ssd1306->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    this->ssd1306->setCursor(coords.x, coords.y);
    this->ssd1306->setTextSize(size);
    this->ssd1306->print(text);
    this->ssd1306->display();
  }

};

extern QueueHandle_t displayQueue;
void startDisplayTask();
