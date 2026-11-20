// STUB de WiFi.h (so o que o projeto usa).
#pragma once
#include "Arduino.h"

typedef enum { WIFI_MODE_NULL = 0, WIFI_MODE_STA, WIFI_MODE_AP } wifi_mode_t;
#define WIFI_STA WIFI_MODE_STA

typedef enum {
  WIFI_POWER_19_5dBm = 78,
  WIFI_POWER_8_5dBm = 34,
  WIFI_POWER_5dBm = 20
} wifi_power_t;

struct WiFiStub {
  wifi_mode_t modo = WIFI_MODE_NULL;
  wifi_power_t potencia = WIFI_POWER_19_5dBm;
  bool mode(wifi_mode_t m) { modo = m; return true; }
  bool disconnect() { return true; }
  bool setTxPower(wifi_power_t p) { potencia = p; return true; }
  uint8_t *macAddress(uint8_t *mac) {
    static const uint8_t fixo[6] = {0xE8, 0xDB, 0x84, 0x0F, 0xAD, 0xB4};
    memcpy(mac, fixo, 6);
    return mac;
  }
};
inline WiFiStub WiFi;
