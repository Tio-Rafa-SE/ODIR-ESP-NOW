// STUB de esp_now.h. Reproduz a MUDANCA DE ASSINATURA do callback de recepcao
// entre o Arduino-ESP32 2.x e 3.x (por isso o sketch precisa tratar os dois).
#pragma once
#include "Arduino.h"
#include "esp_wifi.h"
#include <vector>

#define ESP_NOW_ETH_ALEN 6
#define ESP_NOW_KEY_LEN 16

typedef struct esp_now_peer_info {
  uint8_t peer_addr[ESP_NOW_ETH_ALEN];
  uint8_t lmk[ESP_NOW_KEY_LEN];
  uint8_t channel;
  wifi_interface_t ifidx;
  bool encrypt;
  void *priv;
} esp_now_peer_info_t;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
typedef struct esp_now_recv_info {
  uint8_t *src_addr;
  uint8_t *des_addr;
  void *rx_ctrl;
} esp_now_recv_info_t;
typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t *info, const uint8_t *data, int data_len);
#else
typedef void (*esp_now_recv_cb_t)(const uint8_t *mac_addr, const uint8_t *data, int data_len);
#endif

struct EspNowMundo {
  esp_now_recv_cb_t cbReceber = nullptr;
  std::vector<std::vector<uint8_t>> enviados;     // payloads que a luva mandou
  std::vector<std::vector<uint8_t>> destinos;     // MACs de destino
  int peers = 0;
};
inline EspNowMundo g_espnow;

inline esp_err_t esp_now_init() { return ESP_OK; }
inline esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb) { g_espnow.cbReceber = cb; return ESP_OK; }
inline esp_err_t esp_now_add_peer(const esp_now_peer_info_t *) { g_espnow.peers++; return ESP_OK; }
inline esp_err_t esp_now_send(const uint8_t *mac, const uint8_t *dados, size_t n) {
  g_espnow.enviados.emplace_back(dados, dados + n);
  g_espnow.destinos.emplace_back(mac, mac + 6);
  return ESP_OK;
}

// Gancho de teste: injeta um pacote "vindo do ar" no callback do sketch.
inline void mockReceberPacote(const uint8_t *origem, const uint8_t *dados, int n) {
  if (!g_espnow.cbReceber) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_now_recv_info_t info = {const_cast<uint8_t *>(origem), nullptr, nullptr};
  g_espnow.cbReceber(&info, dados, n);
#else
  g_espnow.cbReceber(origem, dados, n);
#endif
}
