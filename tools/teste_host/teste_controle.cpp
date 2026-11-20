// Simulacao do CONTROLE: roda o codigo REAL do Passo5 (incluido abaixo), move
// os "joysticks" (valores do ADC) e captura os pacotes que iriam pelo ar.
// Serve para as DUAS montagens: o firmware le dois pinos analogicos, e pronto.
#include "Arduino.h"
#include "WiFi.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "teste_comum.h"
#include TESTE_SKETCH

static void ciclo(uint32_t ms) {
  uint32_t alvo = g_mundo.agoraMs + ms;
  while (g_mundo.agoraMs < alvo) {
    uint32_t antes = g_mundo.agoraMs;
    loop();
    if (g_mundo.agoraMs == antes) g_mundo.agoraMs++;
  }
}

static PacoteControle ultimoPacote() {
  PacoteControle p;
  memset(&p, 0, sizeof(p));
  if (!g_espnow.enviados.empty()) memcpy(&p, g_espnow.enviados.back().data(), sizeof(p));
  return p;
}

static void apertarBotao(uint32_t ms) {
  g_mundo.digital[PINO_BOTAO] = LOW;
  ciclo(ms);
  g_mundo.digital[PINO_BOTAO] = HIGH;
  ciclo(60);
}

// Escala o ADC a partir do centro medido: frac em -1..+1 (+ = para o lado ALTO do ADC).
static int adcPara(int centro, float frac) {
  return frac >= 0 ? centro + (int)lroundf(frac * (ADC_BORDA_MAX - centro))
                   : centro + (int)lroundf(frac * (centro - ADC_BORDA_MIN));
}

static const int CENTRO_A = 1987;   // centros "tortos" de proposito: nenhum modulo e perfeito
static const int CENTRO_D = 2046;

int main() {
  g_mundo.agoraMs = 5000;
  g_mundo.analogico[PINO_ACELERADOR] = CENTRO_A;
  g_mundo.analogico[PINO_DIRECAO]    = CENTRO_D;
  setup();

  // --- pinos do codigo original ---
  CHECK(PINO_DIRECAO == 32 && PINO_ACELERADOR == 33 && PINO_BOTAO == 25);

  // --- boot: radio certo, calibrou o centro ---
  CHECK(g_espnow.peers == 1);
  CHECK(g_canalWifi == CANAL_ESPNOW);
  CHECK(abs(g_centroAcel - CENTRO_A) <= 2);
  CHECK(abs(g_centroDir  - CENTRO_D) <= 2);
  CHECK(g_modoMisto == MODO_INICIAL_MISTO);

  // --- joystick(s) parado(s): comando zero, pacote valido, MAC certo ---
  ciclo(200);
  CHECK(!g_espnow.enviados.empty());
  PacoteControle p = ultimoPacote();
  CHECK(pacoteValido(p));
  CHECK(p.acelerador == 0 && p.direcao == 0);
  CHECK(memcmp(g_espnow.destinos.back().data(), MAC_CARRO, 6) == 0);

  // --- ritmo: ~50 pacotes por segundo, sequencia de 1 em 1 ---
  {
    size_t antes = g_espnow.enviados.size();
    ciclo(1000);
    size_t n = g_espnow.enviados.size() - antes;
    CHECK(n >= 45 && n <= 55);
    bool seqOk = true;
    for (size_t i = antes + 1; i < g_espnow.enviados.size(); i++) {
      PacoteControle a, b;
      memcpy(&a, g_espnow.enviados[i - 1].data(), sizeof(a));
      memcpy(&b, g_espnow.enviados[i].data(), sizeof(b));
      if ((uint8_t)(b.sequencia - a.sequencia) != 1) seqOk = false;
    }
    CHECK(seqOk);
  }

  // --- ACELERADOR: ADC baixo = frente (convencao do v1) ---
  g_mundo.analogico[PINO_ACELERADOR] = ADC_BORDA_MIN;       // empurrou "para o lado baixo"
  ciclo(100);
  p = ultimoPacote();
  CHECK(pacoteValido(p) && p.acelerador == 100 && p.direcao == 0);
  g_mundo.analogico[PINO_ACELERADOR] = 4095;                // lado alto = re, e satura em -100
  ciclo(100);
  p = ultimoPacote();
  CHECK(p.acelerador == -100 && p.direcao == 0);

  // --- DIRECAO: ADC alto = direita ---
  g_mundo.analogico[PINO_ACELERADOR] = CENTRO_A;
  g_mundo.analogico[PINO_DIRECAO] = 4095;
  ciclo(100);
  p = ultimoPacote();
  CHECK(p.direcao == 100 && p.acelerador == 0);
  g_mundo.analogico[PINO_DIRECAO] = 0;
  ciclo(100);
  p = ultimoPacote();
  CHECK(p.direcao == -100 && p.acelerador == 0);

  // --- 1 JOYSTICK (os dois eixos no mesmo joystick) e 2 JOYSTICKS (um em cada eixo)
  //     chegam ao firmware do mesmo jeito: eixos INDEPENDENTES, sem um interferir no outro ---
  g_mundo.analogico[PINO_ACELERADOR] = adcPara(CENTRO_A, -1.0f);   // frente a fundo
  g_mundo.analogico[PINO_DIRECAO]    = adcPara(CENTRO_D, +1.0f);   // direita a fundo
  ciclo(100);
  p = ultimoPacote();
  CHECK(p.acelerador == 100 && p.direcao == 100);                   // diagonal do joystick unico
  g_mundo.analogico[PINO_DIRECAO] = CENTRO_D;
  ciclo(100);
  p = ultimoPacote();
  CHECK(p.acelerador == 100 && p.direcao == 0);                     // so o 2o joystick voltou ao centro
  g_mundo.analogico[PINO_ACELERADOR] = CENTRO_A;
  ciclo(100);
  p = ultimoPacote();
  CHECK(p.acelerador == 0 && p.direcao == 0);

  // --- zona morta e proporcionalidade ---
  g_mundo.analogico[PINO_ACELERADOR] = CENTRO_A - (int)(0.05f * (CENTRO_A - ADC_BORDA_MIN));   // 5% do curso
  ciclo(100);
  CHECK(ultimoPacote().acelerador == 0);                            // dentro da zona morta
  g_mundo.analogico[PINO_ACELERADOR] = adcPara(CENTRO_A, -0.55f);   // 55% do curso, zona 10% -> 50%
  ciclo(100);
  CHECK(abs(ultimoPacote().acelerador - 50) <= 2);
  g_mundo.analogico[PINO_ACELERADOR] = CENTRO_A;

  // --- BOTAO: um aperto = UMA troca de modo (o v1 repetia a cada pacote) ---
  bool antes = g_modoMisto;
  apertarBotao(100);
  CHECK(g_modoMisto == !antes);
  CHECK(((ultimoPacote().flags & FLAG_MODO_MISTO) != 0) == g_modoMisto);   // o carro fica sabendo
  apertarBotao(2000);                                                       // segurar 2 s: ainda UMA troca
  CHECK(g_modoMisto == antes);
  CHECK(((ultimoPacote().flags & FLAG_MODO_MISTO) != 0) == g_modoMisto);
  apertarBotao(10);                                                         // "tremida" de 10 ms: ignorada
  CHECK(g_modoMisto == antes);
  apertarBotao(100);
  CHECK(g_modoMisto == !antes);
  apertarBotao(100);
  CHECK(g_modoMisto == antes);

  // --- calibracao: centro fora da faixa = fio errado -> avisa e usa o padrao ---
  {
    g_centroAcel = 1234; g_centroDir = 1234;                  // valores de "marcador"
    g_mundo.analogico[PINO_ACELERADOR] = 100;                 // preso em ~0 (fio solto/pino errado)
    g_mundo.analogico[PINO_DIRECAO]    = 4000;                // preso em ~max (joystick em 5V?)
    calibrarCentro();
    CHECK(g_centroAcel == 1234 && g_centroDir == 1234);       // NAO adotou centros absurdos
  }
  // --- calibracao: alguem mexendo durante a medicao -> tenta de novo e desiste sem travar ---
  {
    static uint32_t aoPassar = 0;
    g_mundo.aoPassarTempo = [](uint32_t t) {
      aoPassar++;
      g_mundo.analogico[PINO_ACELERADOR] = 1500 + (int)((t / 3) % 2) * 1200;   // oscila forte
      g_mundo.analogico[PINO_DIRECAO]    = 2046;
    };
    g_centroAcel = 1111; g_centroDir = 2222;
    uint32_t t0 = g_mundo.agoraMs;
    calibrarCentro();
    g_mundo.aoPassarTempo = nullptr;
    CHECK(g_centroAcel == 1111 && g_centroDir == 2222);       // manteve o que tinha
    CHECK(g_mundo.agoraMs - t0 < 10000);                       // desistiu: nao ficou preso
    (void)aoPassar;
  }
  // --- calibracao boa depois de tudo isso ---
  g_mundo.analogico[PINO_ACELERADOR] = 2010;
  g_mundo.analogico[PINO_DIRECAO]    = 2030;
  calibrarCentro();
  CHECK(abs(g_centroAcel - 2010) <= 2 && abs(g_centroDir - 2030) <= 2);

  return resumo("controle");
}
