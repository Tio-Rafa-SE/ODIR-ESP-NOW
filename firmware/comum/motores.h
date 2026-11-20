// ============================================================================
//  motores.h  -  ODIR ESP-NOW 2.0
//
//  Controle de UM motor DC pela ponte H (L298N), usando PWM nos pinos IN1/IN2.
//
//  A ideia (e por que ela e boa para sala de aula):
//     frente : IN1 = PWM(velocidade)   IN2 = LOW
//     re     : IN1 = LOW               IN2 = PWM(velocidade)
//     parado : IN1 = LOW               IN2 = LOW
//  Assim os jumpers ENA/ENB do L298N continuam INSTALADOS (como no v1 do ODIR) e a
//  velocidade passa a ser controlavel SEM mexer na fiacao. Quem montou o v1
//  nao precisa abrir o carro.
//
//  Compatibilidade de core: o Arduino-ESP32 3.x mudou a API do LEDC (PWM).
//  Aqui os dois jeitos convivem (veja pwmAnexar/pwmEscrever).
//  Ref.: Arduino-ESP32, "Migration from 2.x to 3.0" (secao LEDC).
//
//  Regra de ouro: pino ligado ao LEDC NUNCA recebe digitalWrite() depois.
//  Zero tambem e valor de PWM: pwmEscrever(pino, canal, 0).
//
//  Copia oficial em firmware/comum/ - use tools/sincronizar_comum.sh.
// ============================================================================
#pragma once
#include <Arduino.h>
#include "config_carro.h"
#include "logica.h"

static_assert(PWM_RESOLUCAO_BITS == 8, "motores.h assume duty de 8 bits (0..255)");

// ---- Camada de compatibilidade do PWM -------------------------------------
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  // Core 3.x: o canal e escolhido automaticamente; tudo e feito pelo PINO.
  inline bool pwmAnexar(int pino, uint8_t /*canal*/) {
    return ledcAttach(pino, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
  }
  inline void pwmEscrever(int pino, uint8_t /*canal*/, uint32_t duty) {
    ledcWrite(pino, duty);
  }
#else
  // Core 2.x: precisamos reservar um canal e ligar o pino nele.
  inline bool pwmAnexar(int pino, uint8_t canal) {
    ledcSetup(canal, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    ledcAttachPin(pino, canal);
    return true;
  }
  inline void pwmEscrever(int /*pino*/, uint8_t canal, uint32_t duty) {
    ledcWrite(canal, duty);
  }
#endif

class MotorPonteH {
 public:
  // canalBase: no core 2.x o motor usa os canais canalBase e canalBase+1.
  // Retorna false se o PWM nao pode ser configurado.
  bool iniciar(int pinoIn1, int pinoIn2, bool inverter, uint8_t canalBase) {
    _in1 = pinoIn1;
    _in2 = pinoIn2;
    _inverter = inverter;
    _ch1 = canalBase;
    _ch2 = canalBase + 1;

    // 1) Estado seguro ANTES de qualquer coisa: os dois pinos em LOW.
    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, LOW);

    // 2) Agora entrega os pinos ao PWM e garante duty = 0.
    bool ok = pwmAnexar(_in1, _ch1);
    ok = pwmAnexar(_in2, _ch2) && ok;
    parar();
    return ok;
  }

  // Velocidade em -1.0 (re a fundo) .. +1.0 (frente a fundo).
  void definir(float v) {
    v = limitarF(v, -1.0f, 1.0f);
    int duty = velocidadeParaPwm(fabsf(v), PWM_MINIMO, PWM_MAXIMO);
    definirPwm(v < 0.0f ? -duty : duty);
  }

  // PWM "cru", -255..+255, ja respeitando a inversao do motor.
  // Usado pelo teste de motores para descobrir o PWM minimo.
  void definirPwm(int duty) {
    if (_inverter) duty = -duty;
    if (duty > 255) duty = 255;
    if (duty < -255) duty = -255;
    if (duty > 0) {
      pwmEscrever(_in1, _ch1, (uint32_t)duty);
      pwmEscrever(_in2, _ch2, 0);
    } else if (duty < 0) {
      pwmEscrever(_in1, _ch1, 0);
      pwmEscrever(_in2, _ch2, (uint32_t)(-duty));
    } else {
      parar();
    }
  }

  // Para na hora (os dois lados da ponte em LOW = motor "solto").
  void parar() {
    pwmEscrever(_in1, _ch1, 0);
    pwmEscrever(_in2, _ch2, 0);
  }

 private:
  int _in1 = -1, _in2 = -1;
  uint8_t _ch1 = 0, _ch2 = 1;
  bool _inverter = false;
};
