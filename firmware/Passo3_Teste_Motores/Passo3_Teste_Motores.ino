// ============================================================================
//  ODIR ESP-NOW 2.0  -  Passo 3: testar os motores do carro (sem radio)
//  Prof. Rafael Oliveira (Tio Rafa) - Robotica Paula Souza
//
//  Grave no ESP32 do CARRO. Este sketch roda uma sequencia fixa e imprime o
//  que esta fazendo, para voce CONFERIR com o que as rodas fazem de verdade.
//
//  ATENCAO - SEGURANCA: levante o carro (rodas no ar) ou segure-o! Ele vai
//  se mexer sozinho 5 segundos depois de ligar.
//
//  Este teste usa delay() DE PROPOSITO: e uma sequencia didatica linear. No
//  Passo 4 (o carro de verdade) nao existe delay bloqueante.
//
//  O que conferir:
//    - "MOTOR ESQUERDO frente": a roda ESQUERDA gira para a frente?
//      Se for a DIREITA, os fios dos dois motores estao trocados: troque os
//      pinos no config_carro.h (ou os fios).
//      Se a roda certa gira para TRAS, ponha INVERTER_MOTOR_ESQ = true.
//    - Idem para o motor DIREITO.
//    - "RAMPA DE PWM": veja a partir de qual valor a roda comeca a girar de
//      verdade; anote e coloque em PWM_MINIMO (no config_carro.h).
//
//  Resultado esperado: as 4 etapas (esq, dir, os dois, giros) com o sentido
//  certo; depois a rampa de PWM; depois a sequencia se repete.
// ============================================================================
#include <Arduino.h>
#include "config_carro.h"
#include "logica.h"
#include "motores.h"
#include "utilitarios.h"

static MotorPonteH motorEsq;
static MotorPonteH motorDir;

static void pararTudo() {
  motorEsq.parar();
  motorDir.parar();
}

static void etapa(const char *titulo, float esq, float dir, uint32_t duracaoMs) {
  Serial.printf(">> %s\n", titulo);
  motorEsq.definir(esq);
  motorDir.definir(dir);
  delay(duracaoMs);
  pararTudo();
  delay(1000);
}

static void rampaDePwm() {
  Serial.println(">> RAMPA DE PWM (os dois motores, para frente). Anote quando a roda COMECA a girar.");
  for (int duty = 20; duty <= 255; duty += 15) {
    Serial.printf("   PWM = %3d de 255\n", duty);
    motorEsq.definirPwm(duty);
    motorDir.definirPwm(duty);
    delay(500);
  }
  pararTudo();
  delay(1000);
}

void setup() {
  // Primeiro de tudo: motores em estado seguro.
  bool ok = motorDir.iniciar(PINO_MOTOR_DIR_IN1, PINO_MOTOR_DIR_IN2, INVERTER_MOTOR_DIR, 0);
  ok = motorEsq.iniciar(PINO_MOTOR_ESQ_IN1, PINO_MOTOR_ESQ_IN2, INVERTER_MOTOR_ESQ, 2) && ok;

  iniciarSerial();
  Serial.println();
  Serial.println("=== ODIR ESP-NOW 2.0 - Passo 3: teste dos motores ===");
  Serial.printf("Placa: %s | DIR: IN1=%d IN2=%d | ESQ: IN1=%d IN2=%d\n", PLACA_NOME,
                PINO_MOTOR_DIR_IN1, PINO_MOTOR_DIR_IN2, PINO_MOTOR_ESQ_IN1, PINO_MOTOR_ESQ_IN2);
  if (!ok) Serial.println("ERRO: nao consegui configurar o PWM de algum motor.");

  Serial.println("LEVANTE O CARRO! Comecando em 5 segundos...");
  for (int i = 5; i > 0; i--) {
    Serial.printf("  %d...\n", i);
    delay(1000);
  }
}

void loop() {
  etapa("MOTOR ESQUERDO frente (50%) - a roda ESQUERDA deve girar para a frente", 0.5f, 0.0f, 1500);
  etapa("MOTOR ESQUERDO re (50%)     - a roda ESQUERDA deve girar para tras",     -0.5f, 0.0f, 1500);
  etapa("MOTOR DIREITO frente (50%)  - a roda DIREITA deve girar para a frente",   0.0f, 0.5f, 1500);
  etapa("MOTOR DIREITO re (50%)      - a roda DIREITA deve girar para tras",       0.0f, -0.5f, 1500);
  etapa("OS DOIS para frente (50%)   - o carro anda em linha reta",                0.5f, 0.5f, 1500);

  // Giros no lugar: mistura diferencial com acelerador 0.
  float e, d;
  misturarDiferencial(0.0f, 0.5f, e, d);
  etapa("GIRO para a DIREITA no lugar (esq frente, dir re)", e, d, 1500);
  misturarDiferencial(0.0f, -0.5f, e, d);
  etapa("GIRO para a ESQUERDA no lugar (esq re, dir frente)", e, d, 1500);

  rampaDePwm();

  Serial.println("--- fim da sequencia; repetindo em 5 s (aperte RESET para parar de vez) ---");
  delay(5000);
}
