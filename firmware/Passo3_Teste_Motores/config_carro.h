// ============================================================================
//  config_carro.h  -  ODIR ESP-NOW 2.0  (CARRO / receptor)
//
//  TUDO que voce pode querer ajustar no carro esta AQUI.
//  O resto do codigo nao precisa ser tocado para adaptar o projeto.
//
//  Copia oficial em firmware/comum/ - use tools/sincronizar_comum.sh.
// ============================================================================
#pragma once
#include <Arduino.h>
#include <stdint.h>

#define VERSAO_FIRMWARE "2.0.0"

// ============================================================================
//  0) SO ESP32 CLASSICO
//
//  Este projeto foi escrito e documentado para o ESP32 classico (DevKit com
//  modulo ESP-WROOM-32). "ESP32" e uma familia: S2, S3, C3, C6... tem pinos e
//  comportamentos de boot diferentes. Se voce selecionou outra familia na
//  Arduino IDE, o compilador para aqui de proposito, em vez de deixar o robo
//  andar com o pino errado. (Para o ESP HAND CONTROL, que ja suporta C3, veja
//  o outro projeto do Tio Rafa.)
// ============================================================================
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || \
    defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32H2)
  #error "Este projeto e so para o ESP32 classico (ESP-WROOM-32). Em outra familia, confira o pinout da SUA placa e adapte os pinos antes de usar."
#endif
#define PLACA_NOME "ESP32 classico"

// ============================================================================
//  1) PINAGEM
//
//  Os 4 pinos abaixo sao OS MESMOS do codigo original do ODIR ESP-NOW (video e
//  revista). Quem ja montou o carro nao precisa mexer na fiacao.
//
//  Ligacao no L298N:
//     Motor DIREITO  (OUT1/OUT2): IN1 <- GPIO16,  IN2 <- GPIO17
//     Motor ESQUERDO (OUT3/OUT4): IN3 <- GPIO18,  IN4 <- GPIO19
//  O codigo original nunca mexia em ENA/ENB, entao eles ja estao habilitados
//  (jumpers instalados). Mantenha assim: a velocidade vem do PWM nos pinos IN.
//
//  CONFIRA: GPIO16 e GPIO17 so ficam livres em placas com modulo ESP-WROOM-32.
//  Em modulos ESP32-WROVER (com PSRAM), esses dois pinos sao usados pela memoria
//  e NAO podem acionar motores.
// ============================================================================
constexpr int PINO_MOTOR_DIR_IN1 = 16;
constexpr int PINO_MOTOR_DIR_IN2 = 17;
constexpr int PINO_MOTOR_ESQ_IN1 = 18;
constexpr int PINO_MOTOR_ESQ_IN2 = 19;
constexpr int PINO_LED           = 2;    // LED azul de muitas DevKit. Se a sua nao tem, nada acontece. -1 = sem LED.

// ============================================================================
//  2) SENTIDO DOS MOTORES
//  Rodou o Passo 3 e uma roda gira ao contrario do esperado? Mude para true.
// ============================================================================
constexpr bool INVERTER_MOTOR_ESQ = false;
constexpr bool INVERTER_MOTOR_DIR = false;

// ============================================================================
//  3) PWM (velocidade)
// ============================================================================
constexpr uint32_t PWM_FREQUENCIA_HZ  = 5000;  // acima do que o ouvido incomoda, abaixo do limite do L298N
constexpr uint8_t  PWM_RESOLUCAO_BITS = 8;     // duty de 0 a 255 (nao mude sem revisar motores.h)
constexpr uint8_t  PWM_MINIMO         = 80;    // abaixo disso o motor so "zumbe". Descubra o seu no Passo 3.
constexpr uint8_t  PWM_MAXIMO         = 255;
constexpr uint8_t  LIMITE_VELOCIDADE_PCT = 100; // primeiros testes / sala pequena: use 60

// ============================================================================
//  4) SUAVIDADE E SEGURANCA
// ============================================================================
constexpr uint32_t PERIODO_CONTROLE_MS = 20;    // motores atualizados 50x por segundo
constexpr float    PASSO_ACELERAR      = 0.10f; // quanto a velocidade pode SUBIR por ciclo (0.10 = 0 a 100% em 0,2 s)
constexpr float    PASSO_FREAR         = 0.34f; // quanto pode CAIR por ciclo (0.34 = para em ~60 ms; solta a alavanca, o robo obedece)
constexpr uint32_t TIMEOUT_SINAL_MS    = 400;   // sem pacote valido por este tempo => motores PARAM (o v1 esperava 1000 ms)

// ============================================================================
//  5) MODO "DIRECOES" (o modo simples do v1)
//  A partir de que porcentagem da alavanca (ja depois da zona morta) o carro
//  entende "foi para a frente/re/esquerda/direita".
// ============================================================================
constexpr int LIMIAR_DIRECOES_PCT = 40;

// ============================================================================
//  6) QUEM PODE DIRIGIR ESTE CARRO
//  Por padrao o carro obedece a QUALQUER controle que fale o protocolo certo.
//  Em sala, com varios carros e controles, ligue o filtro e informe o MAC do
//  controle (o Passo 1, gravado no controle, mostra esse MAC).
// ============================================================================
#define FILTRAR_REMETENTE 0
static const uint8_t MAC_CONTROLE_PERMITIDO[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// ============================================================================
//  7) DEPURACAO
// ============================================================================
constexpr bool     DEBUG_SERIAL        = true;
constexpr uint32_t DEBUG_INTERVALO_MS  = 500;
