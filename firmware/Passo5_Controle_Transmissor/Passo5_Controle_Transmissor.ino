// ============================================================================
//  ODIR ESP-NOW 2.0  -  Passo 5: CONTROLE (transmissor)
//  Prof. Rafael Oliveira (Tio Rafa) - Robotica Paula Souza
//  Mecatronica Jovem, edicao 19 - "ODIR com ESP32 e ESP-NOW"
//
//  Base: codigo original de Ujwal Nandanwar, adaptado e comentado por Rafael
//  Oliveira (v1). Esta versao 2.0 foi reescrita e documentada; o credito
//  original continua aqui, onde deve estar.
//
//  Le o(s) joystick(s) e manda, 50 vezes por segundo, dois numeros para o carro
//  via ESP-NOW:
//      acelerador : -100 (re a fundo) .. +100 (frente a fundo)
//      direcao    : -100 (esquerda)   .. +100 (direita)
//
//  UM firmware, DUAS montagens (so muda a fiacao - veja o manual):
//      1 joystick  : o X e o Y do mesmo joystick
//      2 joysticks : um para frente/tras (VRy), outro para esquerda/direita (VRx)
//
//  Ao ligar: NAO TOQUE nos joysticks por ~1 segundo. O controle mede o "centro"
//  de cada eixo (cada joystick tem o seu) e passa a descontar esse valor.
//
//  Botao (SW do joystick, ligado em GPIO25): troca o MODO de conducao.
//      DIRECOES : frente / re / gira esq. / gira dir., velocidade cheia (v1 simples)
//      MISTO    : acelerador + direcao juntos, velocidade proporcional
//  O modo vai dentro de cada pacote; o carro so obedece.
//
//  LED (se houver): aceso = modo MISTO | piscando lento = modo DIRECOES
//                   piscando rapido = calibracao
//
//  Resultado esperado (Monitor Serial, 115200):
//      Calibrando o centro... NAO toque nos joysticks.
//      Centro: acel=1987 dir=2046
//      Controle pronto. Modo: DIRECOES (aperte o botao para trocar).
//      [DIRECOES] adc A=1990 D=2051 -> acel=+0 dir=+0 | seq=250 falhasEnvio=0
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config_controle.h"
#include "protocolo.h"
#include "logica.h"
#include "utilitarios.h"

// ---- Estado ----
static int      g_centroAcel = CENTRO_PADRAO;
static int      g_centroDir  = CENTRO_PADRAO;
static bool     g_modoMisto  = MODO_INICIAL_MISTO;
static uint8_t  g_sequencia  = 0;
static uint32_t g_falhasEnvio = 0;
static int      g_ultAdcA = 0, g_ultAdcD = 0;
static int8_t   g_ultAcel = 0, g_ultDir = 0;

// ============================================================================
//  RADIO
// ============================================================================
static bool macZerado(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) if (mac[i] != 0) return false;
  return true;
}

// Liga o radio no modo "estacao" (sem conectar a nenhuma rede), fixa o canal e
// cadastra o carro como destino. Controle e carro PRECISAM usar o mesmo canal
// (CANAL_ESPNOW, em protocolo.h).
static bool iniciarRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_wifi_set_channel(CANAL_ESPNOW, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("AVISO: nao consegui fixar o canal Wi-Fi.");
  }
  if (esp_now_init() != ESP_OK) return false;

  // No ESP-NOW todo destino precisa ser cadastrado como "peer" antes do envio.
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, MAC_CARRO, 6);
  peer.channel = CANAL_ESPNOW;
  peer.ifidx   = WIFI_IF_STA;
  peer.encrypt = false;     // criptografia e um desafio do manual (PMK/LMK)
  return esp_now_add_peer(&peer) == ESP_OK;
}

// Monta o pacote de 7 bytes (veja docs/PROTOCOLO.md) e envia ao carro.
// A sequencia sobe de 1 em 1: o carro usa isso para contar pacotes perdidos.
static void enviarPacote(int8_t acelerador, int8_t direcao, bool modoMisto) {
  PacoteControle p;
  p.magic      = PROTOCOLO_MAGIC;
  p.versao     = PROTOCOLO_VERSAO;
  p.sequencia  = g_sequencia++;
  p.acelerador = acelerador;
  p.direcao    = direcao;
  p.flags      = modoMisto ? FLAG_MODO_MISTO : 0;
  p.checksum   = calcularChecksum(p);
  // esp_now_send devolve erro so para problemas LOCAIS (peer nao cadastrado,
  // fila cheia...). Ele nao garante que o carro recebeu: o controle fala "no
  // escuro" e quem faz o failsafe e o carro.
  if (esp_now_send(MAC_CARRO, reinterpret_cast<const uint8_t *>(&p), sizeof(p)) != ESP_OK) g_falhasEnvio++;
}

// ============================================================================
//  JOYSTICK
// ============================================================================
// Media de algumas leituras: o ADC do ESP32 e "nervoso", a media acalma.
static int lerEixo(int pino) {
  long soma = 0;
  for (int i = 0; i < AMOSTRAS_POR_LEITURA; i++) soma += analogRead(pino);
  return (int)(soma / AMOSTRAS_POR_LEITURA);
}

// Mede o centro dos dois eixos, com os joysticks SOLTOS.
// Se a leitura variar demais, alguem mexeu: tenta de novo. Se o centro cair
// fora da faixa aceitavel, algo esta errado na fiacao (ou o joystick esta preso):
// avisa e usa o valor padrao, em vez de sair andando.
static void calibrarCentro() {
  Serial.println("Calibrando o centro... NAO toque nos joysticks.");
  for (int tentativa = 1; tentativa <= CALIBRACAO_TENTATIVAS; tentativa++) {
    long somaA = 0, somaD = 0;
    int minA = 4095, maxA = 0, minD = 4095, maxD = 0;
    for (int i = 0; i < CALIBRACAO_AMOSTRAS; i++) {
      int a = lerEixo(PINO_ACELERADOR);
      int d = lerEixo(PINO_DIRECAO);
      somaA += a; somaD += d;
      if (a < minA) minA = a;
      if (a > maxA) maxA = a;
      if (d < minD) minD = d;
      if (d > maxD) maxD = d;
      ledEscrever(PINO_LED, piscando(millis(), 200));
      delay(10);
    }
    if ((maxA - minA) > CALIBRACAO_TOLERANCIA || (maxD - minD) > CALIBRACAO_TOLERANCIA) {
      Serial.printf("  oscilou demais (A: %d..%d, D: %d..%d). Solte os joysticks. Tentativa %d de %d...\n",
                    minA, maxA, minD, maxD, tentativa, CALIBRACAO_TENTATIVAS);
      continue;
    }
    int cA = (int)(somaA / CALIBRACAO_AMOSTRAS);
    int cD = (int)(somaD / CALIBRACAO_AMOSTRAS);
    bool okA = (cA >= CENTRO_MIN_ACEITO && cA <= CENTRO_MAX_ACEITO);
    bool okD = (cD >= CENTRO_MIN_ACEITO && cD <= CENTRO_MAX_ACEITO);
    if (okA) g_centroAcel = cA; else Serial.printf("AVISO: centro do ACELERADOR (%d) fora do normal. Confira a fiacao (GPIO%d). Usando %d.\n", cA, PINO_ACELERADOR, CENTRO_PADRAO);
    if (okD) g_centroDir  = cD; else Serial.printf("AVISO: centro da DIRECAO (%d) fora do normal. Confira a fiacao (GPIO%d). Usando %d.\n", cD, PINO_DIRECAO, CENTRO_PADRAO);
    Serial.printf("Centro: acel=%d dir=%d\n", g_centroAcel, g_centroDir);
    return;
  }
  Serial.println("AVISO: nao consegui calibrar (joystick mexendo ou fio solto). Usando o centro padrao.");
}

// ============================================================================
//  BOTAO  (um aperto = um evento; ignora tremida do contato)
// ============================================================================
static bool botaoFoiApertado(uint32_t agora) {
  if (PINO_BOTAO < 0) return false;
  static bool     estavaApertado = false;
  static bool     jaDisparou = false;
  static uint32_t inicioAperto = 0;

  bool apertado = (digitalRead(PINO_BOTAO) == LOW);     // INPUT_PULLUP: apertar = LOW
  if (!apertado) { estavaApertado = false; return false; }

  if (!estavaApertado) {              // acabou de apertar
    estavaApertado = true;
    jaDisparou = false;
    inicioAperto = agora;
    return false;
  }
  if (!jaDisparou && (agora - inicioAperto) >= BOTAO_DEBOUNCE_MS) {
    jaDisparou = true;                // dispara UMA vez por aperto (o v1 repetia a cada pacote)
    return true;
  }
  return false;
}

// ============================================================================
//  LED E DEBUG
// ============================================================================
// LED de estado (se PINO_LED >= 0): fixo = MISTO, pisca lento = DIRECOES.
// O mesmo padrao do LED do carro, para voce ver os dois modos "casando".
static void atualizarLed(uint32_t agora) {
  ledEscrever(PINO_LED, g_modoMisto ? true : piscando(agora, 1000));
}

static void imprimirDebug() {
  Serial.printf("[%s] adc A=%4d D=%4d -> acel=%+4d dir=%+4d | seq=%u falhasEnvio=%lu\n",
                g_modoMisto ? "MISTO" : "DIRECOES", g_ultAdcA, g_ultAdcD, g_ultAcel, g_ultDir,
                (unsigned)g_sequencia, (unsigned long)g_falhasEnvio);
}

static void erroFatal(const char *mensagem) {
  for (;;) {
    Serial.printf("ERRO FATAL: %s\n", mensagem);
    for (int i = 0; i < 10; i++) {
      ledEscrever(PINO_LED, (i & 1) == 0);
      delay(200);
    }
  }
}

// ============================================================================
//  SETUP / LOOP
// ============================================================================
void setup() {
  iniciarSerial();
  ledIniciar(PINO_LED);
  if (PINO_BOTAO >= 0) pinMode(PINO_BOTAO, INPUT_PULLUP);

  Serial.println();
  Serial.printf("=== ODIR ESP-NOW %s - CONTROLE | %s ===\n", VERSAO_FIRMWARE, PLACA_NOME);

  if (macZerado(MAC_CARRO)) {
    erroFatal("MAC_CARRO ainda e 00:00:..: rode o Passo1 no carro e cole o MAC no config_controle.h.");
  }
  if (!iniciarRadio()) erroFatal("ESP-NOW nao iniciou (ou nao consegui cadastrar o carro).");

  // O centro e medido AGORA, com os joysticks soltos: por isso o manual pede
  // para nao tocar neles na hora de ligar.
  calibrarCentro();
  Serial.printf("Controle pronto. Modo: %s (aperte o botao para trocar).\n", g_modoMisto ? "MISTO" : "DIRECOES");
}

void loop() {
  static uint32_t tEnvio = 0, tDebug = 0;
  uint32_t agora = millis();

  if (botaoFoiApertado(agora)) {
    g_modoMisto = !g_modoMisto;
    Serial.printf(">> MODO %s\n", g_modoMisto ? "MISTO (acelerador + direcao)" : "DIRECOES (frente/re/giro)");
  }
  atualizarLed(agora);

  // ---- 1 leitura + 1 envio a cada PERIODO_ENVIO_MS ----
  if (agora - tEnvio >= PERIODO_ENVIO_MS) {
    tEnvio = agora;

    g_ultAdcA = lerEixo(PINO_ACELERADOR);
    g_ultAdcD = lerEixo(PINO_DIRECAO);

    // Passo a passo da conta: ADC (0..4095) -> eixo de -1 a +1 (usando o centro
    // medido e as bordas) -> sinal conforme a orientacao -> percentual de -100
    // a +100 com a zona morta aplicada (logica.h).
    float sa = normalizarEixo(g_ultAdcA, g_centroAcel, ADC_BORDA_MIN, ADC_BORDA_MAX);
    float sd = normalizarEixo(g_ultAdcD, g_centroDir,  ADC_BORDA_MIN, ADC_BORDA_MAX);
    if (!INVERTER_ACELERADOR) sa = -sa;      // convencao do v1: ADC baixo = frente
    if (INVERTER_DIRECAO)     sd = -sd;      // convencao do v1: ADC alto  = direita

    g_ultAcel = eixoParaPercentual(sa, ZONA_MORTA);
    g_ultDir  = eixoParaPercentual(sd, ZONA_MORTA);
    enviarPacote(g_ultAcel, g_ultDir, g_modoMisto);
  }

  if (DEBUG_SERIAL && (agora - tDebug >= DEBUG_INTERVALO_MS)) {
    tDebug = agora;
    imprimirDebug();
  }
  delay(1);   // 1 ms de folga para o sistema (Wi-Fi, watchdog); nao afeta os 50 envios por segundo
}
