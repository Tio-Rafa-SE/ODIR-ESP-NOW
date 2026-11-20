// ============================================================================
//  ODIR ESP-NOW 2.0  -  Passo 2: testar o(s) joystick(s) (sem radio)
//  Prof. Rafael Oliveira (Tio Rafa) - Robotica Paula Souza
//
//  Grave no ESP32 do CONTROLE (com 1 joystick OU com 2 joysticks: o teste e o
//  mesmo). Abra o Monitor Serial a 115200.
//
//  O que este sketch faz: le os dois eixos e o botao, e mostra
//      adc      : o numero cru do conversor (0 a 4095)
//      pct      : o comando que o controle mandaria (-100 a +100), usando o
//                 CENTRO PADRAO (no Passo 5 o centro e medido ao ligar)
//      botao    : APERTADO ou solto
//
//  O que conferir (resultado esperado):
//    1. Joystick PARADO: adc perto de 2048 (entre ~1700 e ~2400) e pct = 0.
//    2. Empurre o acelerador PARA A FRENTE do robo: pct do ACELERADOR deve ficar
//       POSITIVO (+). Se ficou negativo, troque INVERTER_ACELERADOR no
//       config_controle.h.
//    3. Empurre a direcao PARA A DIREITA: pct da DIRECAO deve ficar POSITIVO.
//       Se nao, troque INVERTER_DIRECAO.
//    4. Aperte o botao: aparece "APERTADO".
//
//  Alertas automaticos:
//    - adc travado em 0 ou perto de 4095: fio solto, pino errado, ou joystick
//      ligado em 5V em vez de 3V3.
//    - adc "pulando" sem voce tocar: o pino esta no ar (fio solto) - um pino
//      analogico sem ligacao le qualquer coisa. Reencaixe o jumper.
//
//  Para ver os eixos em grafico, ponha MODO_PLOTTER = true e abra
//  Ferramentas > Plotter Serial.
// ============================================================================
#include <Arduino.h>
#include "config_controle.h"
#include "logica.h"
#include "utilitarios.h"

constexpr bool MODO_PLOTTER = false;   // true = imprime so "acel:..,dir:.." para o Plotter Serial

static int lerEixo(int pino) {
  long soma = 0;
  for (int i = 0; i < AMOSTRAS_POR_LEITURA; i++) soma += analogRead(pino);
  return (int)(soma / AMOSTRAS_POR_LEITURA);
}

// Mesmo calculo do Passo 5, mas com o centro padrao.
static int8_t paraPercentual(int adc, bool inverter, bool convencaoAdcBaixoPositivo) {
  float s = normalizarEixo(adc, CENTRO_PADRAO, ADC_BORDA_MIN, ADC_BORDA_MAX);
  if (convencaoAdcBaixoPositivo) s = -s;   // acelerador: ADC baixo = frente (+)
  if (inverter) s = -s;
  return eixoParaPercentual(s, ZONA_MORTA);
}

static const char *alerta(int adc) {
  if (adc <= 20)   return "  <-- ALERTA: preso em 0 (fio solto/pino errado?)";
  if (adc >= 4075) return "  <-- ALERTA: preso no maximo (3V3 no pino? joystick em 5V?)";
  return "";
}

void setup() {
  iniciarSerial();
  ledIniciar(PINO_LED);
  if (PINO_BOTAO >= 0) pinMode(PINO_BOTAO, INPUT_PULLUP);
  Serial.println();
  Serial.println("=== ODIR ESP-NOW 2.0 - Passo 2: teste dos joysticks ===");
  Serial.printf("Placa: %s | ACELERADOR=GPIO%d DIRECAO=GPIO%d BOTAO=GPIO%d\n", PLACA_NOME,
                PINO_ACELERADOR, PINO_DIRECAO, PINO_BOTAO);
  Serial.println("Mexa os joysticks e confira os sinais (+/-). Sem tocar: adc perto de 2048.");
}

void loop() {
  static uint32_t tImprime = 0;
  uint32_t agora = millis();

  if (agora - tImprime >= 150) {
    tImprime = agora;
    int adcA = lerEixo(PINO_ACELERADOR);
    int adcD = lerEixo(PINO_DIRECAO);
    int pctA = paraPercentual(adcA, INVERTER_ACELERADOR, true);
    int pctD = paraPercentual(adcD, INVERTER_DIRECAO, false);
    bool apertado = (PINO_BOTAO >= 0) && (digitalRead(PINO_BOTAO) == LOW);

    if (MODO_PLOTTER) {
      Serial.printf("acel:%d,dir:%d\n", pctA, pctD);
    } else {
      Serial.printf("ACEL adc=%4d pct=%+4d | DIR adc=%4d pct=%+4d | botao: %s%s%s\n",
                    adcA, pctA, adcD, pctD, apertado ? "APERTADO" : "solto",
                    alerta(adcA), alerta(adcD));
    }
  }
  ledEscrever(PINO_LED, piscando(agora, 1000));   // LED piscando = o sketch esta vivo
  delay(1);
}
