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
public:
  ScreenSlider(Adafruit_SSD1306 *oled, RoboEyes<Adafruit_SSD1306> *eyes)
    : oled(oled), eyes(eyes), current_screen(SCREEN_EYES) {
    weekly_text[0] = '\0';
    pace_text[0] = '\0';
  }

  void set_usage_data(const char *weekly, const char *pace) {
    strncpy(weekly_text, weekly, MAX_TEXT_LEN - 1);
    weekly_text[MAX_TEXT_LEN - 1] = '\0';
    strncpy(pace_text, pace, MAX_TEXT_LEN - 1);
    pace_text[MAX_TEXT_LEN - 1] = '\0';
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
        oled->print(weekly_text);
        oled->setCursor(5, 30);
        oled->print(pace_text);
        oled->display();
        break;
    }
  }

  ScreenType get_current_screen() const {
    return current_screen;
  }

};
