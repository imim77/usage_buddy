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

#define BUTTON_DEBOUNCE_MS 200

static void setLed(bool on) {
  digitalWrite(LED_BUILTIN, (LED_ACTIVE_LOW ? !on : on) ? HIGH : LOW);
}

static SemaphoreHandle_t buttonSemaphore;
static volatile uint32_t lastButtonPress = 0;

void IRAM_ATTR buttonISR() {
  uint32_t now = millis();
  if (now - lastButtonPress >= BUTTON_DEBOUNCE_MS) {
    lastButtonPress = now;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
      portYIELD_FROM_ISR();
    }
  }
}

void ButtonTask(void *parameters) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  
  for (;;) {
    if (xSemaphoreTake(buttonSemaphore, portMAX_DELAY) == pdPASS) {
      if (displayQueue != nullptr) {
        bool buttonPressed = true;
        xQueueSend(displayQueue, &buttonPressed, 0);
      }
      if (httpQueue != nullptr) {
        bool triggerFetch = true;
        xQueueSend(httpQueue, &triggerFetch, 0);
      }
    }
  }
}

static void fetchAndParseUsage() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  RequestClient client({SERVER_HOST, SERVER_PORT, REQUEST_TIMEOUT_MS});
  int code;
  String payload = client.get(SERVER_PATH, code);
  Serial.printf("[HTTP] GET %s -> %d\n", SERVER_PATH, code);

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

void HttpTask(void *parameters) {
  bool dummy;
  
  for (;;) {
    if (xQueueReceive(httpQueue, &dummy, portMAX_DELAY) == pdPASS) {
      fetchAndParseUsage();
    }
  }
}

void KeepWiFiAlive(void *parameters) {
  enum State { DISCONNECTED, CONNECTING, CONNECTED, RETRY_DELAY };
  State state = DISCONNECTED;
  unsigned long stateStart = 0;
  bool blink = false;
  
  for (;;) {
    unsigned long now = millis();
    
    switch (state) {
      case DISCONNECTED:
        Serial.println("[WiFi] connecting...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        state = CONNECTING;
        stateStart = now;
        blink = false;
        break;
        
      case CONNECTING:
        blink = !blink;
        setLed(blink);
        
        if (WiFi.status() == WL_CONNECTED) {
          setLed(true);
          Serial.print("[WiFi] connected, IP: ");
          Serial.println(WiFi.localIP());
          state = CONNECTED;
          stateStart = now;
        } else if (now - stateStart >= WIFI_TIMEOUT_MS) {
          Serial.println("[WiFi] timeout, retrying...");
          WiFi.disconnect(true);
          state = RETRY_DELAY;
          stateStart = now;
          blink = false;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
        break;
        
      case CONNECTED:
        setLed(true);
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("[WiFi] disconnected");
          state = DISCONNECTED;
        } else {
          vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
        break;
        
      case RETRY_DELAY:
        blink = !blink;
        setLed(blink);
        
        if (now - stateStart >= WIFI_RETRY_DELAY_MS) {
          state = DISCONNECTED;
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
        break;
    }
  }
}

void setup() { 
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  setLed(false);
  
  displayQueue = xQueueCreate(10, sizeof(bool));
  httpQueue = xQueueCreate(10, sizeof(bool));
  sliderMutex = xSemaphoreCreateMutex();
  buttonSemaphore = xSemaphoreCreateBinary();
  
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
