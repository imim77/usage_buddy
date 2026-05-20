#pragma once
#include <stdint.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

enum ScreenType {
  SCREEN_EYES,
  SCREEN_BUTTON_PRESSED
};

class ScreenSlider {
private:
  Adafruit_SSD1306 *oled;
  RoboEyes<Adafruit_SSD1306> *eyes;
  ScreenType current_screen;
public:
  ScreenSlider(Adafruit_SSD1306 *oled, RoboEyes<Adafruit_SSD1306> *eyes)
    : oled(oled), eyes(eyes), current_screen(SCREEN_EYES) {}

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
        oled->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        oled->setCursor(10, 20);
        oled->setTextSize(2);
        oled->print("Button pressed");
        oled->display();
        break;
    }
  }

  ScreenType get_current_screen() const {
    return current_screen;
  }

};
