#include <Arduino.h>
#include <WiFi.h>
#include "env_config.h"

#ifndef WIFI_TIMEOUT_MS
#define WIFI_TIMEOUT_MS 20000
#endif

void KeepWiFiAlive(void* paramaters){
  for(;;){
    if(WiFi.status() == WL_CONNECTED){
        Serial.println("WiFi still connected");
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        continue;
    }

    Serial.println("WiFi connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startAttemptTime = millis();

    while(WiFi.status() != WL_CONNECTED && millis()-startAttemptTime < WIFI_TIMEOUT_MS){
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        vTaskDelay(200 / portTICK_PERIOD_MS);
    };

    digitalWrite(LED_BUILTIN, LOW);

    if(WiFi.status() != WL_CONNECTED){
        Serial.println("[WiFi] FAILED");
        vTaskDelay(20000 / portTICK_PERIOD_MS);
        continue;
    }

    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("[WiFi] CONNECTED");
    Serial.println(WiFi.localIP());

  }
}



void setup() { 
}

void loop() {
  // put your main code here, to run repeatedly:
}
