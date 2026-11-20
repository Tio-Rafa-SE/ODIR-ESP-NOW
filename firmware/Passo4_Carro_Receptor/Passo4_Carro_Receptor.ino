// ============================================================================
//  ODIR ESP-NOW 2.0  -  Passo 4: CARRO (receptor)
//  Prof. Rafael Oliveira (Tio Rafa) - Robotica Paula Souza
//  Mecatronica Jovem, edicao 19 - "ODIR com ESP32 e ESP-NOW"
//
//  Base: codigo original de Ujwal Nandanwar, adaptado por Rafael Oliveira
//  (v1). Esta versao 2.0 foi reescrita e documentada; o credito original
//  continua aqui, onde deve estar.
//
//  Recebe pelo ESP-NOW os comandos do controle e move os dois motores.
//
//      controle (ESP32 + joystick) --ESP-NOW--> carro (ESP32 + L298N + 2 motores)
//
//  Dois modos (o CONTROLE escolhe, com o botao do joystick):
//    DIRECOES : como o modo "simples" do v1. Frente, re, gira esq., gira dir.
//               Velocidade cheia. Frente/re ganham de esquerda/direita.
//    MISTO    : acelerador + direcao ao mesmo tempo, velocidade proporcional
//               ao quanto voce empurra (curvas suaves, "dirigindo de verdade").
//
//  O que mudou em relacao ao v1 (resumo; detalhes no manual):
//    - O callback do radio so COPIA o pacote. Quem mexe nos motores e o
//      loop(), em ritmo fixo de 50 Hz (o v1 mexia nos motores e imprimia no
//      Serial DENTRO do callback, a cada pacote).
//    - Quem decide o modo e o CONTROLE (flag no pacote). No v1 o carro
//      invertia o modo a cada pacote recebido com o botao apertado.
//    - No v1, o 2o modo usava map() com inteiros e, com o joystick no centro,
//      o resultado era "os dois motores para a frente" (veja o manual). Aqui
//      o modo MISTO usa mistura diferencial com numeros decimais.
//    - Velocidade por PWM (o v1 era liga/desliga), com rampa de aceleracao e
//      freio rapido.
//    - Pacote validado (tamanho, assinatura, versao, checksum, faixa).
//    - Failsafe de 400 ms (o v1 esperava 1000 ms).
//    - Compila nos cores Arduino-ESP32 2.x e 3.x (o v1 so no 3.x).
//
//  Resultado esperado (Monitor Serial, 115200):
//      MAC desta placa: XX:XX:...        <- o que voce colou no config_controle.h
//      Radio pronto. Aguardando o controle...
//      [SEM SINAL] ...                   <- enquanto o controle estiver desligado
//      [MISTO] acel=+60 dir=+0 | E=+0.60 D=+0.60 ...   <- controle ligado
//  LED (se houver): aceso = modo MISTO | piscando lento = modo DIRECOES
//                   piscando rapido = sem sinal do controle
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config_carro.h"
#include "protocolo.h"
#include "logica.h"
#include "motores.h"
#include "utilitarios.h"

// ---- Tipos (ficam antes de qualquer funcao, por causa do pre-processador da IDE) ----
enum class Estado : uint8_t { SEM_SINAL, DIRECOES, MISTO };

struct Comando {
  int8_t   acelerador;
  int8_t   direcao;
  uint8_t  flags;
  uint32_t recebidoEm;   // millis() do ultimo pacote valido
  bool     existe;       // ja chegou algum pacote valido desde que ligou?
};

// Tudo que o callback do radio escreve e o loop() le fica NUMA struct so,
// protegida por um "mutex" leve (portMUX). Assim o loop() nunca enxerga um
// comando "pela metade".
struct Recepcao {
  Comando  comando;
  uint32_t ok;         // pacotes validos recebidos
  uint32_t ruins;      // pacotes descartados (tamanho, assinatura, checksum, remetente)
  uint32_t perdidos;   // pacotes que sumiram no caminho (furos na sequencia)
};

// ---- Estado compartilhado entre o callback do radio e o loop() ----
static Recepcao     g_rx  = {{0, 0, 0, 0, false}, 0, 0, 0};
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
// So o callback mexe nestes dois:
static uint8_t g_ultimaSequencia = 0;
static bool    g_temSequencia    = false;

// ---- Estado so do loop() ----
static MotorPonteH g_motorEsq;
static MotorPonteH g_motorDir;
static Estado g_estado = Estado::SEM_SINAL;
static float  g_atualEsq = 0.0f, g_atualDir = 0.0f;   // velocidade aplicada agora (apos a rampa)
static int8_t g_ultAcel = 0, g_ultDir = 0;            // so para o debug

// ============================================================================
//  RADIO
// ============================================================================

// Conta um pacote descartado. O contador e lido pelo loop() (so para o debug),
// por isso o acesso passa pelo mutex.
static void contarPacoteRuim() {
  portENTER_CRITICAL(&g_mux);
  g_rx.ruins++;
  portEXIT_CRITICAL(&g_mux);
}

// Chamado pela tarefa de Wi-Fi a cada pacote recebido (via aoReceber, abaixo).
// Regra de ouro: ser RAPIDO e nao imprimir nada aqui (a documentacao da
// Espressif pede para nao fazer operacoes demoradas dentro do callback).
// Ele so VALIDA e COPIA o comando; quem mexe nos motores e o loop().
static void processarPacote(const uint8_t *origem, const uint8_t *dados, int tamanho) {
  (void)origem;   // so usado se FILTRAR_REMETENTE estiver ligado

  if (tamanho != (int)sizeof(PacoteControle)) { contarPacoteRuim(); return; }
  PacoteControle p;
  memcpy(&p, dados, sizeof(p));
  if (!pacoteValido(p)) { contarPacoteRuim(); return; }

#if FILTRAR_REMETENTE
  if (memcmp(origem, MAC_CONTROLE_PERMITIDO, 6) != 0) { contarPacoteRuim(); return; }
#endif

  // Quantos pacotes sumiram no caminho? (a sequencia pulou mais de 1)
  uint32_t furos = 0;
  if (g_temSequencia) furos = (uint8_t)(p.sequencia - g_ultimaSequencia - 1);
  g_ultimaSequencia = p.sequencia;
  g_temSequencia = true;

  uint32_t agora = millis();
  portENTER_CRITICAL(&g_mux);
  g_rx.comando.acelerador = p.acelerador;
  g_rx.comando.direcao    = p.direcao;
  g_rx.comando.flags      = p.flags;
  g_rx.comando.recebidoEm = agora;
  g_rx.comando.existe     = true;
  g_rx.ok++;
  g_rx.perdidos += furos;
  portEXIT_CRITICAL(&g_mux);
}

// A assinatura do callback mudou no Arduino-ESP32 3.x (recebe uma struct de
// informacao em vez do MAC cru). Os dois jeitos convivem aqui.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
static void aoReceber(const esp_now_recv_info_t *info, const uint8_t *dados, int tamanho) {
  processarPacote(info->src_addr, dados, tamanho);
}
#else
static void aoReceber(const uint8_t *mac, const uint8_t *dados, int tamanho) {
  processarPacote(mac, dados, tamanho);
}
#endif

// Liga o radio no modo "estacao" (sem conectar a nenhuma rede) e fixa o canal.
// Controle e carro PRECISAM estar no mesmo canal (CANAL_ESPNOW, em protocolo.h).
static bool iniciarRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();   // garante que nao ha tentativa de reconexao mexendo no canal
  if (esp_wifi_set_channel(CANAL_ESPNOW, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("AVISO: nao consegui fixar o canal Wi-Fi.");
  }
  if (esp_now_init() != ESP_OK) return false;
  return esp_now_register_recv_cb(aoReceber) == ESP_OK;
}

// ============================================================================
//  MOTORES
// ============================================================================
// Tira uma "foto" consistente do estado do radio: copia tudo de uma vez, com o
// mutex travado, para o loop() trabalhar sem o callback mudar os valores no meio.
static Recepcao lerRecepcao() {
  Recepcao r;
  portENTER_CRITICAL(&g_mux);
  r = g_rx;
  portEXIT_CRITICAL(&g_mux);
  return r;
}

// Para tudo na hora, sem rampa, e zera a velocidade "lembrada" pela rampa
// (assim, quando o sinal volta, o carro parte do zero e nao de onde parou).
static void pararImediato() {
  g_atualEsq = 0.0f;
  g_atualDir = 0.0f;
  g_motorEsq.parar();
  g_motorDir.parar();
}

// Roda a cada PERIODO_CONTROLE_MS. Aqui mora o failsafe.
static void atualizarMotores() {
  Comando c = lerRecepcao().comando;
  int32_t idade = (int32_t)(millis() - c.recebidoEm);   // com sinal: pacote "do futuro" nao vira timeout falso
  bool temSinal = c.existe && idade <= (int32_t)TIMEOUT_SINAL_MS;

  if (!temSinal) {
    pararImediato();                       // sem sinal: parar NA HORA, sem rampa
    g_estado = Estado::SEM_SINAL;
    g_ultAcel = 0;
    g_ultDir = 0;
    return;
  }

  const bool misto = (c.flags & FLAG_MODO_MISTO) != 0;
  g_estado = misto ? Estado::MISTO : Estado::DIRECOES;
  g_ultAcel = c.acelerador;
  g_ultDir  = c.direcao;

  float alvoEsq, alvoDir;
  if (misto) {
    misturarDiferencial(c.acelerador / 100.0f, c.direcao / 100.0f, alvoEsq, alvoDir);
  } else {
    direcoesSimples(c.acelerador, c.direcao, LIMIAR_DIRECOES_PCT, alvoEsq, alvoDir);
  }
  const float limite = LIMITE_VELOCIDADE_PCT / 100.0f;
  alvoEsq *= limite;
  alvoDir *= limite;

  g_atualEsq = aproximarComFreio(g_atualEsq, alvoEsq, PASSO_ACELERAR, PASSO_FREAR);
  g_atualDir = aproximarComFreio(g_atualDir, alvoDir, PASSO_ACELERAR, PASSO_FREAR);
  g_motorEsq.definir(g_atualEsq);
  g_motorDir.definir(g_atualDir);
}

// ============================================================================
//  LED E DEBUG
// ============================================================================
// LED de estado (se PINO_LED >= 0): fixo = MISTO, pisca lento = DIRECOES,
// pisca rapido = sem sinal. Serve para saber o estado sem abrir o Monitor Serial.
static void atualizarLed(uint32_t agora) {
  switch (g_estado) {
    case Estado::MISTO:     ledEscrever(PINO_LED, true); break;
    case Estado::DIRECOES:  ledEscrever(PINO_LED, piscando(agora, 1000)); break;
    case Estado::SEM_SINAL: ledEscrever(PINO_LED, piscando(agora, 250)); break;
  }
}

static void imprimirDebug() {
  Recepcao rx = lerRecepcao();
  const char *nome = g_estado == Estado::MISTO ? "MISTO" : (g_estado == Estado::DIRECOES ? "DIRECOES" : "SEM SINAL");
  Serial.printf("[%s] acel=%+d dir=%+d | E=%+.2f D=%+.2f | ok=%lu ruins=%lu perdidos=%lu\n",
                nome, g_ultAcel, g_ultDir, g_atualEsq, g_atualDir,
                (unsigned long)rx.ok, (unsigned long)rx.ruins, (unsigned long)rx.perdidos);
}

static void erroFatal(const char *mensagem) {
  pararImediato();
  for (;;) {
    Serial.printf("ERRO FATAL: %s\n", mensagem);
    for (int i = 0; i < 10; i++) {                // pisca rapido por 2 s
      ledEscrever(PINO_LED, (i & 1) == 0);
      delay(200);
    }
  }
}

// ============================================================================
//  SETUP / LOOP
// ============================================================================
void setup() {
  // 1) Motores PRIMEIRO, em estado seguro (pinos em LOW, PWM em zero).
  //    Os dois ultimos numeros (0 e 2) sao os canais de PWM (LEDC) de cada motor;
  //    cada motor usa 2 canais consecutivos (um por pino IN), entao 0-1 e 2-3.
  bool motoresOk = g_motorDir.iniciar(PINO_MOTOR_DIR_IN1, PINO_MOTOR_DIR_IN2, INVERTER_MOTOR_DIR, 0);
  motoresOk = g_motorEsq.iniciar(PINO_MOTOR_ESQ_IN1, PINO_MOTOR_ESQ_IN2, INVERTER_MOTOR_ESQ, 2) && motoresOk;
  ledIniciar(PINO_LED);

  // 2) Serial e apresentacao.
  iniciarSerial();
  Serial.println();
  Serial.printf("=== ODIR ESP-NOW %s - CARRO | %s ===\n", VERSAO_FIRMWARE, PLACA_NOME);
  if (!motoresOk) erroFatal("PWM dos motores nao configurado (confira os pinos no config_carro.h).");

  // 3) Radio.
  if (!iniciarRadio()) erroFatal("ESP-NOW nao iniciou.");
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("MAC desta placa: %02X:%02X:%02X:%02X:%02X:%02X (canal %u)\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], (unsigned)CANAL_ESPNOW);
  Serial.println("Radio pronto. Aguardando o controle...");
}

void loop() {
  static uint32_t tControle = 0, tDebug = 0;
  uint32_t agora = millis();

  if (agora - tControle >= PERIODO_CONTROLE_MS) {
    tControle = agora;
    atualizarMotores();
  }
  atualizarLed(agora);

  if (DEBUG_SERIAL && (agora - tDebug >= DEBUG_INTERVALO_MS)) {
    tDebug = agora;
    imprimirDebug();
  }
  delay(1);   // 1 ms de folga para o resto do sistema (Wi-Fi, watchdog). Nao afeta o controle.
}
