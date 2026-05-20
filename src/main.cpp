#include <Arduino.h>
#include <WiFi.h>
#include "display.h"
#include "env_config.h"

#ifndef WIFI_TIMEOUT_MS
#define WIFI_TIMEOUT_MS 20000
#endif

#ifndef WIFI_RETRY_DELAY_MS
#define WIFI_RETRY_DELAY_MS 5000
#endif

#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 0
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN 5
#endif

static void setLed(bool on) {
  digitalWrite(LED_BUILTIN, (LED_ACTIVE_LOW ? !on : on) ? HIGH : LOW);
}

void KeepWiFiAlive(void *parameters) {
  for(;;){
    if(WiFi.status() == WL_CONNECTED){
        setLed(true);
        Serial.println("[WiFi] still connected");
        vTaskDelay(20000 / portTICK_PERIOD_MS);
        continue;
    }

    Serial.println("[WiFi] connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startAttemptTime = millis();
    bool blinkState = false;

    while(WiFi.status() != WL_CONNECTED && millis()-startAttemptTime < WIFI_TIMEOUT_MS){
        blinkState = !blinkState;
        setLed(blinkState);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    };

    if(WiFi.status() != WL_CONNECTED){
        Serial.println("[WiFi] failed, retrying...");
        WiFi.disconnect(true);
        unsigned long retryStart = millis();
        while (millis() - retryStart < WIFI_RETRY_DELAY_MS) {
          blinkState = !blinkState;
          setLed(blinkState);
          vTaskDelay(250 / portTICK_PERIOD_MS);
        }
        continue;
    }

    setLed(true);
    Serial.print("[WiFi] connected, IP: ");
    Serial.println(WiFi.localIP());

  }
}



void setup() { 
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setLed(false);
  delay(300);
  Serial.println("[BOOT] starting...");
  startDisplayTask();

  xTaskCreatePinnedToCore(
        KeepWiFiAlive,
        "Keep WiFi alive",
        5000,
        NULL,
        1,
        NULL,
        CONFIG_ARDUINO_RUNNING_CORE
  );

}

void loop() {
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW && displayQueue != nullptr) {
    bool buttonPressed = true;
    xQueueSend(displayQueue, &buttonPressed, 0);
  }

  lastButtonState = buttonState;
  delay(10);
}
