#pragma once
#include <stdint.h>
#include <HTTPClient.h>

struct HttpClientConfig {
  const char *server_url;
  uint16_t port;
  uint32_t timeout_ms;
};

class RequestClient {
private:
  HttpClientConfig config;
public:
  RequestClient(const HttpClientConfig &config)
    : config(config) {}

  int get(const char *path) {
    HTTPClient http;
    http.setTimeout(config.timeout_ms);
    http.begin(config.server_url, config.port, path);
    int code = http.GET();
    http.end();
    return code;
  }
};
