// Teste de fumaca: o sketch compila e roda setup() + algumas voltas de loop().
#include "Arduino.h"
#include "WiFi.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include TESTE_SKETCH

int main() {
  setup();
  for (int i = 0; i < 20; i++) loop();
  return 0;
}
