#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "display.h"
#include "env_config.h"
#include "http_client.h"
#include "usage_display.h"

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

#define BUTTON_DEBOUNCE_MS 50

static void setLed(bool on) {
  digitalWrite(LED_BUILTIN, (LED_ACTIVE_LOW ? !on : on) ? HIGH : LOW);
}

enum WiFiState {
  WIFI_DISCONNECTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_RETRY_DELAY
};

static WiFiState wifiState = WIFI_DISCONNECTED;
static unsigned long stateStartTime = 0;
static bool ledBlinkState = false;

void KeepWiFiAlive(void *parameters) {
  for (;;) {
    unsigned long now = millis();
    
    switch (wifiState) {
      case WIFI_DISCONNECTED:
        Serial.println("[WiFi] connecting...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        wifiState = WIFI_CONNECTING;
        stateStartTime = now;
        ledBlinkState = false;
        break;
        
      case WIFI_CONNECTING:
        ledBlinkState = !ledBlinkState;
        setLed(ledBlinkState);
        
        if (WiFi.status() == WL_CONNECTED) {
          setLed(true);
          Serial.print("[WiFi] connected, IP: ");
          Serial.println(WiFi.localIP());
          wifiState = WIFI_CONNECTED;
          stateStartTime = now;
        } else if (now - stateStartTime >= WIFI_TIMEOUT_MS) {
          Serial.println("[WiFi] connection timeout, retrying...");
          WiFi.disconnect(true);
          wifiState = WIFI_RETRY_DELAY;
          stateStartTime = now;
          ledBlinkState = false;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
        break;
        
      case WIFI_CONNECTED:
        setLed(true);
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("[WiFi] disconnected");
          wifiState = WIFI_DISCONNECTED;
        } else {
          vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
        break;
        
      case WIFI_RETRY_DELAY:
        ledBlinkState = !ledBlinkState;
        setLed(ledBlinkState);
        
        if (now - stateStartTime >= WIFI_RETRY_DELAY_MS) {
          wifiState = WIFI_DISCONNECTED;
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
        break;
    }
  }
}

void HttpTask(void *parameters) {
  RequestClient client({SERVER_HOST, SERVER_PORT, REQUEST_TIMEOUT_MS});
  
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      int code;
      String payload = client.get(SERVER_PATH, code);
      Serial.printf("[HTTP] GET %s -> %d\n", SERVER_PATH, code);
      Serial.printf("[HTTP] Response: %s\n", payload.c_str());

      if (code == 200) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          const char *weekly = doc["usage"]["weekly"]["formatted"];
          const char *pace = doc["pace"]["weekly"];
          if (weekly && pace) {
            set_usage_data(weekly, pace);
          }
        } else {
          Serial.printf("[JSON] Parse failed: %s\n", error.c_str());
        }
      }
    }
    vTaskDelay(REQUEST_INTERVAL_MS / portTICK_PERIOD_MS);
  }
}

void ButtonTask(void *parameters) {
  static bool lastButtonState = HIGH;
  static unsigned long lastStateChange = 0;
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  for (;;) {
    bool buttonState = digitalRead(BUTTON_PIN);
    unsigned long now = millis();
    
    if (buttonState != lastButtonState) {
      lastStateChange = now;
    }
    
    if (now - lastStateChange >= BUTTON_DEBOUNCE_MS) {
      if (lastButtonState == HIGH && buttonState == LOW) {
        if (displayQueue != nullptr) {
          bool buttonPressed = true;
          xQueueSend(displayQueue, &buttonPressed, 0);
        }
      }
    }
    
    lastButtonState = buttonState;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() { 
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  setLed(false);
  
  displayQueue = xQueueCreate(10, sizeof(bool));
  httpQueue = xQueueCreate(10, sizeof(bool));
  sliderMutex = xSemaphoreCreateMutex();
  
  vTaskDelay(300 / portTICK_PERIOD_MS);
  Serial.println("[BOOT] starting...");
  
  startDisplayTask();

  xTaskCreatePinnedToCore(
        KeepWiFiAlive,
        "Keep WiFi alive",
        4096,
        NULL,
        2,
        NULL,
        CONFIG_ARDUINO_RUNNING_CORE
  );

  xTaskCreatePinnedToCore(
        HttpTask,
        "HTTP Task",
        4096,
        NULL,
        1,
        NULL,
        CONFIG_ARDUINO_RUNNING_CORE
  );
  
  xTaskCreatePinnedToCore(
        ButtonTask,
        "Button Task",
        2048,
        NULL,
        3,
        NULL,
        CONFIG_ARDUINO_RUNNING_CORE
  );
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
