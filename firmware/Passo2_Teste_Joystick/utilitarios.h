// ============================================================================
//  utilitarios.h  -  ODIR ESP-NOW 2.0
//
//  Pecinhas pequenas usadas por todos os sketches: Serial e LED.
//
//  Copia oficial em firmware/comum/ - use tools/sincronizar_comum.sh.
// ============================================================================
#pragma once
#include <Arduino.h>

// Inicia o Serial de um jeito que funciona nas DUAS familias de placa.
//
//  - ESP32 classico: Serial passa pelo chip conversor USB-serial da placa.
//  - ESP32-C3: o USB e do proprio chip ("USB CDC"). Na Arduino IDE e preciso
//    marcar  Ferramentas > USB CDC On Boot > Enabled  para o Serial aparecer
//    no computador. Sem isso, o programa roda, mas o Monitor Serial fica mudo.
//
//  Com USB CDC e SEM computador ligado (carro a bateria), uma escrita no Serial
//  poderia travar o loop esperando alguem ler. setTxTimeoutMs(0) manda
//  descartar em vez de esperar - assim o carro nunca "congela" por causa de
//  um Serial.print esquecido.
inline void iniciarSerial(unsigned long baud = 115200) {
  Serial.begin(baud);
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT)
  Serial.setTxTimeoutMs(0);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 2000) delay(10);  // espera o PC abrir a porta (max 2 s)
#endif
}

// LED opcional: pino negativo = "nao tenho LED", e a funcao vira no-op.
inline void ledIniciar(int pino) {
  if (pino >= 0) {
    pinMode(pino, OUTPUT);
    digitalWrite(pino, LOW);
  }
}
inline void ledEscrever(int pino, bool ligado) {
  if (pino >= 0) digitalWrite(pino, ligado ? HIGH : LOW);
}

// true durante metade de cada periodo (para piscar sem delay()).
inline bool piscando(uint32_t agoraMs, uint32_t periodoMs) {
  return ((agoraMs / (periodoMs / 2)) & 1U) == 0;
}
