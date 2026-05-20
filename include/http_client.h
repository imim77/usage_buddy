#pragma once
#include <stdint.h>
#include <HTTPClient.h>
#include "env_config.h"

#define SERVER_PORT 3000
#define SERVER_PATH "/codex/usage"
#define REQUEST_TIMEOUT_MS 5000
#define REQUEST_INTERVAL_MS 30000

struct HttpClientConfig {
  const char *host;
  uint16_t port;
  uint32_t timeout_ms;
};

class RequestClient {
private:
  HttpClientConfig config;
public:
  RequestClient(const HttpClientConfig &config)
    : config(config) {}

  String get(const char *path, int &code) {
    HTTPClient http;
    http.setTimeout(config.timeout_ms);
    http.begin(config.host, config.port, path);
    code = http.GET();
    String payload = http.getString();
    http.end();
    return payload;
  }
};
