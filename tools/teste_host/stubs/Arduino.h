// STUB de Arduino.h para testar os sketches NO COMPUTADOR (g++), sem placa.
// Imita a API do Arduino-ESP32 o suficiente para compilar e simular o codigo.
// Nao e o core real: ver docs/MANUAL.md, capitulo "O que foi (e o que nao foi) testado".
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

// Versao do core simulada: compile com -DESP_ARDUINO_VERSION_MAJOR=2 ou 3.
#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 3
#endif

#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
typedef uint8_t byte;

// Estado "do mundo" que os testes podem ler e escrever.
struct MockMundo {
  uint32_t agoraMs = 0;
  int      digital[64];     // valor lido por digitalRead / escrito por digitalWrite
  int      analogico[64];   // valor 0..4095 devolvido por analogRead (o teste escreve aqui)
  uint32_t pwm[64];         // duty atual por PINO
  int      canalParaPino[32];
  bool     silencioso = true;
  void   (*aoPassarTempo)(uint32_t) = nullptr;   // gancho de teste: chamado a cada ms simulado
  MockMundo() {
    for (auto &d : digital) d = HIGH;   // botao com pull-up: solto = HIGH
    for (auto &a : analogico) a = 2048; // joystick no centro
    for (auto &p : pwm) p = 0;
    for (auto &c : canalParaPino) c = -1;
  }
};
inline MockMundo g_mundo;

inline uint32_t millis() { return g_mundo.agoraMs; }
inline void delay(uint32_t ms) {                                  // delay "instantaneo": so avanca o relogio simulado
  for (uint32_t i = 0; i < ms; i++) {
    g_mundo.agoraMs++;
    if (g_mundo.aoPassarTempo) g_mundo.aoPassarTempo(g_mundo.agoraMs);
  }
}
inline void pinMode(int, int) {}
inline void digitalWrite(int pino, int v) { g_mundo.digital[pino] = v; }
inline int  digitalRead(int pino) { return g_mundo.digital[pino]; }
inline int  analogRead(int pino) { return g_mundo.analogico[pino]; }

struct SerialStub {
  void begin(unsigned long) {}
  void setTxTimeoutMs(uint32_t) {}
  explicit operator bool() const { return true; }
  void println() { if (!g_mundo.silencioso) puts(""); }
  void println(const char *s) { if (!g_mundo.silencioso) puts(s); }
  void print(const char *s) { if (!g_mundo.silencioso) fputs(s, stdout); }
  int printf(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    if (g_mundo.silencioso) return 0;
    va_list a; va_start(a, fmt); int r = vprintf(fmt, a); va_end(a); return r;
  }
};
inline SerialStub Serial;

// ---- LEDC: SO a API do core escolhido existe (como na vida real) ----------
#if ESP_ARDUINO_VERSION_MAJOR >= 3
inline bool ledcAttach(uint8_t pino, uint32_t, uint8_t) { g_mundo.pwm[pino] = 0; return true; }
inline bool ledcWrite(uint8_t pino, uint32_t duty) { g_mundo.pwm[pino] = duty; return true; }
#else
inline double ledcSetup(uint8_t, uint32_t, uint8_t) { return 0; }
inline void ledcAttachPin(uint8_t pino, uint8_t canal) { g_mundo.canalParaPino[canal] = pino; g_mundo.pwm[pino] = 0; }
inline void ledcWrite(uint8_t canal, uint32_t duty) { g_mundo.pwm[g_mundo.canalParaPino[canal]] = duty; }
#endif

// ---- Secao critica (no PC nao ha concorrencia) ------------------------------
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m)  ((void)(m))
