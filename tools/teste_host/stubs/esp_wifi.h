// STUB de esp_wifi.h
#pragma once
#include "Arduino.h"
typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#define ESP_FAIL -1
#endif
typedef enum { WIFI_SECOND_CHAN_NONE = 0 } wifi_second_chan_t;
typedef enum { WIFI_IF_STA = 0, WIFI_IF_AP = 1 } wifi_interface_t;
inline int g_canalWifi = 0;
inline esp_err_t esp_wifi_set_channel(uint8_t canal, wifi_second_chan_t) { g_canalWifi = canal; return ESP_OK; }
