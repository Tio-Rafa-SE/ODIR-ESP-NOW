// ============================================================================
//  config_controle.h  -  ODIR ESP-NOW 2.0  (CONTROLE / transmissor)
//
//  TUDO que voce pode querer ajustar no controle esta AQUI.
//
//  O MESMO firmware serve para as duas montagens:
//      - 1 JOYSTICK : o X e o Y do MESMO joystick (veja docs/img/ligacao_controle_1_joystick.svg)
//      - 2 JOYSTICKS: um joystick so para frente/tras e outro so para esquerda/direita
//                     (veja docs/img/ligacao_controle_2_joysticks.svg)
//  Para o codigo isso e igual: ele le DOIS pinos analogicos, "acelerador" e
//  "direcao". O que muda e QUEM esta ligado em cada pino. Nao precisa mexer
//  no codigo para trocar de montagem.
//
//  Copia oficial em firmware/comum/ - use tools/sincronizar_comum.sh.
// ============================================================================
#pragma once
#include <Arduino.h>
#include <stdint.h>

#define VERSAO_FIRMWARE "2.0.0"

// ============================================================================
//  0) SO ESP32 CLASSICO (veja a explicacao no config_carro.h)
// ============================================================================
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || \
    defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32H2)
  #error "Este projeto e so para o ESP32 classico (ESP-WROOM-32). Em outra familia, confira o pinout da SUA placa e adapte os pinos antes de usar."
#endif
#define PLACA_NOME "ESP32 classico"

// ============================================================================
//  1) PARA QUEM VAI O PACOTE: o MAC do CARRO
//
//  1. Grave o Passo 1 na placa do CARRO.
//  2. Copie a linha "MAC (codigo)" do Monitor Serial para ca.
//  Enquanto estiver tudo zero, o controle se RECUSA a iniciar (de proposito:
//  assim ninguem fica horas procurando por que "nao funciona").
//
//  Nao copie o MAC de outro robo, nem o do video: cada placa tem o seu.
// ============================================================================
static const uint8_t MAC_CARRO[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// ============================================================================
//  2) PINAGEM
//
//  Os 3 pinos abaixo sao OS MESMOS do codigo original (X=32, Y=33, botao=25).
//
//  Por que GPIO32 e GPIO33 para os eixos? Eles sao do conversor ADC1 do ESP32.
//  O ADC2 (GPIO 0, 2, 4, 12-15, 25-27) NAO funciona enquanto o Wi-Fi/ESP-NOW
//  esta ligado. Ou seja: o GPIO25 pode ser BOTAO (digital), mas nunca eixo.
//
//  ACELERADOR (frente/tras)  <- GPIO33   (o "Y" do joystick, VRy)
//  DIRECAO    (esq/dir)      <- GPIO32   (o "X" do joystick, VRx)
//  BOTAO (troca de modo)     <- GPIO25   (pino SW do joystick, ligado ao GND ao apertar)
//
//  1 JOYSTICK : VRy -> GPIO33,  VRx -> GPIO32,  SW -> GPIO25
//  2 JOYSTICKS: joystick A (frente/tras): VRy -> GPIO33
//               joystick B (esq/dir)    : VRx -> GPIO32
//               SW de UM dos dois        -> GPIO25
//  ALIMENTE OS JOYSTICKS COM 3V3, NUNCA COM 5V: um eixo de 5V no pino do
//  ESP32 pode danificar a placa.
// ============================================================================
constexpr int PINO_ACELERADOR = 33;
constexpr int PINO_DIRECAO    = 32;
constexpr int PINO_BOTAO      = 25;   // -1 = sem botao (o modo fica fixo em MODO_INICIAL_MISTO)
constexpr int PINO_LED        = 2;    // LED azul de muitas DevKit. -1 = sem LED.

// ============================================================================
//  3) ORIENTACAO DOS EIXOS
//
//  Convencao mantida do v1: ADC BAIXO no eixo do acelerador = FRENTE, e
//  ADC ALTO no eixo da direcao = DIREITA. Rode o Passo 2 e confira. Se o seu
//  joystick ficou montado "ao contrario" na caixa, nao mexa nos fios:
//  troque para true.
// ============================================================================
constexpr bool INVERTER_ACELERADOR = false;
constexpr bool INVERTER_DIRECAO    = false;

// ============================================================================
//  4) SENSIBILIDADE
// ============================================================================
constexpr float ZONA_MORTA = 0.10f;        // 10% do curso: dentro disso a alavanca "nao conta". Joystick velho/folgado? Aumente (0.15).
constexpr int   ADC_BORDA_MIN = 150;       // o ADC do ESP32 satura nas pontas: chamamos de 100% um pouco antes
constexpr int   ADC_BORDA_MAX = 3950;
constexpr int   AMOSTRAS_POR_LEITURA = 4;  // media de N leituras por eixo (tira o "ruido" do ADC)

// ============================================================================
//  5) CALIBRACAO DO CENTRO (acontece ao ligar: NAO TOQUE nos joysticks)
// ============================================================================
constexpr int CENTRO_PADRAO         = 2048;  // usado se a calibracao falhar
constexpr int CENTRO_MIN_ACEITO     = 1400;  // centro fora desta faixa = fio errado, joystick preso ou ligado em 5V
constexpr int CENTRO_MAX_ACEITO     = 2700;
constexpr int CALIBRACAO_AMOSTRAS   = 64;
constexpr int CALIBRACAO_TOLERANCIA = 250;   // se a leitura variar mais que isso, voce mexeu: tenta de novo
constexpr int CALIBRACAO_TENTATIVAS = 5;

// ============================================================================
//  6) COMUNICACAO E BOTAO
// ============================================================================
constexpr uint32_t PERIODO_ENVIO_MS     = 20;   // 50 pacotes por segundo (o v1 mandava 20 por segundo, e 2 quando o botao estava apertado)
constexpr uint32_t BOTAO_DEBOUNCE_MS    = 30;   // ignora "tremida" do contato
constexpr bool     MODO_INICIAL_MISTO   = false; // false = "direcoes" (como no video) | true = "acelerador + direcao"

// ============================================================================
//  7) DEPURACAO
// ============================================================================
constexpr bool     DEBUG_SERIAL        = true;
constexpr uint32_t DEBUG_INTERVALO_MS  = 250;
