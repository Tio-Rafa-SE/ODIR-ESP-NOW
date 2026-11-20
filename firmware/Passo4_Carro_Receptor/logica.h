// ============================================================================
//  logica.h  -  ODIR ESP-NOW 2.0
//
//  A "matematica" do projeto, em funcoes PURAS (sem hardware, sem Arduino).
//  Isso e proposital: funcao pura se testa no computador, sem placa nenhuma.
//  (Veja tools/teste_host/ - la essas funcoes sao testadas de verdade.)
//
//  Copia oficial em firmware/comum/ - use tools/sincronizar_comum.sh.
// ============================================================================
#pragma once
#include <math.h>
#include <stdint.h>

inline float limitarF(float v, float minimo, float maximo) {
  return v < minimo ? minimo : (v > maximo ? maximo : v);
}

// ----------------------------------------------------------------------------
//  1) Leitura do ADC (0..4095) -> posicao da alavanca em -1..+1
//
//  Um joystick e um par de potenciometros. No repouso a leitura NAO e
//  exatamente 2048: cada modulo (e cada ESP32) tem o seu "centro". Por isso o
//  controle MEDE o centro ao ligar e passa o valor para ca.
//
//  adcMin/adcMax sao as "bordas uteis": o ADC do ESP32 nao e perfeito perto
//  das pontas (satura), entao chamamos de 100% um pouco ANTES do fim do curso.
//  Resultado:  adc = centro -> 0 | adc >= adcMax -> +1 | adc <= adcMin -> -1
//  (os dois lados sao escalonados separadamente, porque o centro raramente
//  fica no meio exato).
// ----------------------------------------------------------------------------
inline float normalizarEixo(int adc, int centro, int adcMin, int adcMax) {
  if (adc >= centro) {
    int faixa = adcMax - centro;
    if (faixa <= 0) return 0.0f;
    return limitarF((float)(adc - centro) / (float)faixa, 0.0f, 1.0f);
  }
  int faixa = centro - adcMin;
  if (faixa <= 0) return 0.0f;
  return -limitarF((float)(centro - adc) / (float)faixa, 0.0f, 1.0f);
}

// ----------------------------------------------------------------------------
//  2) Posicao (-1..+1) -> comando em porcentagem (-100..+100), com zona morta
//
//  zona : fracao do curso em que "nada acontece" (a alavanca nunca volta
//         EXATAMENTE ao zero). Fora dela o comando sobe de 0 a 100% de forma
//         linear (sem "degrau" na saida da zona morta).
// ----------------------------------------------------------------------------
inline int8_t eixoParaPercentual(float s, float zona) {
  zona = limitarF(zona, 0.0f, 0.95f);
  float a = fabsf(s);
  if (a <= zona) return 0;
  if (a > 1.0f) a = 1.0f;
  int v = (int)lroundf((a - zona) / (1.0f - zona) * 100.0f);
  if (v > 100) v = 100;
  return (int8_t)(s < 0.0f ? -v : v);
}

// ----------------------------------------------------------------------------
//  3) Modo MISTO ("tank drive"): acelerador + direcao -> velocidade de cada lado
//
//  Entradas  : acel e dir em -1..+1   (+dir = virar para a direita)
//  Saidas    : esq e dirOut em -1..+1
//
//      esquerda = acel + dir        direita = acel - dir
//
//  Exemplos:  acel=1, dir=0  -> os dois lados a 100%  (reta)
//             acel=0, dir=1  -> esq +1, dir -1        (gira no lugar)
//             acel=1, dir=1  -> esq 1,  dir 0         (curva fechada)
//  Se algum lado passar de +-1, os dois sao reduzidos juntos (mantem a "forma"
//  da curva em vez de cortar o excesso).
// ----------------------------------------------------------------------------
inline void misturarDiferencial(float acel, float dir, float &esq, float &dirOut) {
  float e = acel + dir;
  float d = acel - dir;
  float m = fmaxf(fabsf(e), fabsf(d));
  if (m > 1.0f) { e /= m; d /= m; }
  esq = e;
  dirOut = d;
}

// ----------------------------------------------------------------------------
//  4) Modo DIRECOES (o "simples" do v1): quatro movimentos, velocidade cheia.
//
//  Prioridade igual a do v1: frente/re ganham de esquerda/direita.
//     acel >= limiar  -> frente         acel <= -limiar -> re
//     dir  >= limiar  -> gira direita   dir  <= -limiar -> gira esquerda
//     senao           -> parado
//  limiar : em porcentagem (0..100), JA depois da zona morta.
// ----------------------------------------------------------------------------
inline void direcoesSimples(int acel, int dir, int limiar, float &esq, float &dirOut) {
  esq = 0.0f;
  dirOut = 0.0f;
  if (acel >= limiar)       { esq =  1.0f; dirOut =  1.0f; }
  else if (acel <= -limiar) { esq = -1.0f; dirOut = -1.0f; }
  else if (dir >= limiar)   { esq =  1.0f; dirOut = -1.0f; }   // gira para a direita
  else if (dir <= -limiar)  { esq = -1.0f; dirOut =  1.0f; }   // gira para a esquerda
}

// ----------------------------------------------------------------------------
//  5) Rampa de aceleracao e de freio
//
//  Anda "atual" na direcao de "alvo". Acelerar usa "passoAcel"; reduzir a
//  velocidade (ou inverter o sentido) usa "passoFreio", maior, para o carro
//  responder rapido quando voce solta a alavanca - mas sem o tranco instantaneo
//  que derruba a tensao da bateria e pode resetar o ESP32.
// ----------------------------------------------------------------------------
inline float aproximar(float atual, float alvo, float passo) {
  if (alvo > atual + passo) return atual + passo;
  if (alvo < atual - passo) return atual - passo;
  return alvo;
}

inline float aproximarComFreio(float atual, float alvo, float passoAcel, float passoFreio) {
  bool freando = (alvo * atual < 0.0f) || (fabsf(alvo) < fabsf(atual));
  return aproximar(atual, alvo, freando ? passoFreio : passoAcel);
}

// ----------------------------------------------------------------------------
//  6) Velocidade (0..1) -> duty de PWM, pulando a "zona preguicosa" do motor.
//  Motor DC de brinquedo nao gira com 5% de PWM: ele zumbe e nao anda.
//  Entao mapeamos 0..1 para [pwmMin..pwmMax], e 0 continua sendo 0.
// ----------------------------------------------------------------------------
inline uint8_t velocidadeParaPwm(float u, uint8_t pwmMin, uint8_t pwmMax) {
  if (u < 0.02f) return 0;
  if (u > 1.0f) u = 1.0f;
  return (uint8_t)lroundf((float)pwmMin + (float)(pwmMax - pwmMin) * u);
}
