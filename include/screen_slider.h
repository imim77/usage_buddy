#pragma once
#include <stdint.h>
#include <string.h>
#include <Adafruit_SSD1306.h>
#ifdef DEFAULT
#undef DEFAULT
#endif
#include <FluxGarage_RoboEyes.h>

#define MAX_TEXT_LEN 32

enum ScreenType {
  SCREEN_EYES,
  SCREEN_BUTTON_PRESSED
};

class ScreenSlider {
private:
  Adafruit_SSD1306 *oled;
  RoboEyes<Adafruit_SSD1306> *eyes;
  ScreenType current_screen;
  char weekly_text[MAX_TEXT_LEN];
  char pace_text[MAX_TEXT_LEN];
  uint8_t usage_percent;

  void draw_progress_bar(uint8_t percent) {
    const uint8_t bar_x = 5;
    const uint8_t bar_y = 50;
    const uint8_t bar_w = 118;
    const uint8_t bar_h = 10;

    oled->drawRect(bar_x, bar_y, bar_w, bar_h, SSD1306_WHITE);

    uint8_t fill_w = (bar_w - 2) * percent / 100;
    if (fill_w > 0) {
      oled->fillRect(bar_x + 1, bar_y + 1, fill_w, bar_h - 2, SSD1306_WHITE);
    }

    char pct_str[8];
    snprintf(pct_str, sizeof(pct_str), "%u%%", percent);
    oled->setTextColor(SSD1306_BLACK);
    oled->setCursor(bar_x + (bar_w / 2) - 10, bar_y + 1);
    oled->setTextSize(1);
    oled->print(pct_str);
  }

public:
  ScreenSlider(Adafruit_SSD1306 *oled, RoboEyes<Adafruit_SSD1306> *eyes)
    : oled(oled), eyes(eyes), current_screen(SCREEN_EYES), usage_percent(0) {
    weekly_text[0] = '\0';
    pace_text[0] = '\0';
  }

  void set_usage_data(const char *weekly, const char *pace, uint8_t percent) {
    strncpy(weekly_text, weekly, MAX_TEXT_LEN - 1);
    weekly_text[MAX_TEXT_LEN - 1] = '\0';
    strncpy(pace_text, pace, MAX_TEXT_LEN - 1);
    pace_text[MAX_TEXT_LEN - 1] = '\0';
    usage_percent = percent;
  }

  void next_screen() {
    current_screen = (current_screen == SCREEN_EYES) ? SCREEN_BUTTON_PRESSED : SCREEN_EYES;
  }

  void render() {
    switch (current_screen) {
      case SCREEN_EYES:
        eyes->update();
        break;
      case SCREEN_BUTTON_PRESSED:
        oled->clearDisplay();
        oled->setTextColor(SSD1306_WHITE);
        oled->setCursor(5, 10);
        oled->setTextSize(1);
        oled->print("Weekly usage\n");
        oled->print(weekly_text);
        oled->setCursor(5, 30);
        oled->print(pace_text);
        draw_progress_bar(usage_percent);
        oled->display();
        break;
    }
  }

  ScreenType get_current_screen() const {
    return current_screen;
  }

};
