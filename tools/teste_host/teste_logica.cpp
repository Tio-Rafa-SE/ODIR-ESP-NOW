// Testes da logica PURA (sem Arduino, sem stubs): firmware/comum/logica.h e protocolo.h
#include "logica.h"
#include "protocolo.h"
#include "teste_comum.h"
#include <string.h>

int main() {
  // ---- normalizarEixo: centro "torto" (1987), bordas 150..3950 ----
  CHECK_NEAR(normalizarEixo(1987, 1987, 150, 3950), 0.0, 1e-6);
  CHECK_NEAR(normalizarEixo(3950, 1987, 150, 3950), 1.0, 1e-6);
  CHECK_NEAR(normalizarEixo(4095, 1987, 150, 3950), 1.0, 1e-6);   // satura, nao passa de 1
  CHECK_NEAR(normalizarEixo(150, 1987, 150, 3950), -1.0, 1e-6);
  CHECK_NEAR(normalizarEixo(0, 1987, 150, 3950), -1.0, 1e-6);
  CHECK_NEAR(normalizarEixo(2968, 1987, 150, 3950), 0.5, 0.01);   // meio caminho para cima
  CHECK_NEAR(normalizarEixo(1068, 1987, 150, 3950), -0.5, 0.01);  // meio caminho para baixo
  CHECK_NEAR(normalizarEixo(100, 100, 150, 3950), 0.0, 1e-6);     // centro degenerado nao divide por zero
  CHECK_NEAR(normalizarEixo(50, 100, 150, 3950), 0.0, 1e-6);

  // ---- eixoParaPercentual: zona morta de 10% ----
  CHECK(eixoParaPercentual(0.0f, 0.10f) == 0);
  CHECK(eixoParaPercentual(0.05f, 0.10f) == 0);
  CHECK(eixoParaPercentual(-0.10f, 0.10f) == 0);                  // na borda da zona: ainda zero
  CHECK(eixoParaPercentual(1.0f, 0.10f) == 100);
  CHECK(eixoParaPercentual(-1.0f, 0.10f) == -100);
  CHECK(eixoParaPercentual(0.55f, 0.10f) == 50);                  // sobe linear a partir da zona
  CHECK(eixoParaPercentual(-0.55f, 0.10f) == -50);
  CHECK(eixoParaPercentual(3.0f, 0.10f) == 100);                  // nunca passa de 100
  CHECK(eixoParaPercentual(1.0f, 0.99f) == 100);                  // zona absurda e limitada (sem divisao por zero)
  // monotonia: a saida nunca diminui quando a alavanca avanca
  int8_t ant = -101; bool mono = true;
  for (int i = -100; i <= 100; i++) { int8_t v = eixoParaPercentual(i / 100.0f, 0.10f); if (v < ant) mono = false; ant = v; }
  CHECK(mono);

  // ---- direcoesSimples (modo "simples" do v1) ----
  float e, d;
  direcoesSimples(100, 0, 40, e, d);   CHECK(e == 1 && d == 1);       // frente
  direcoesSimples(-100, 0, 40, e, d);  CHECK(e == -1 && d == -1);     // re
  direcoesSimples(0, 100, 40, e, d);   CHECK(e == 1 && d == -1);      // gira direita: esq frente, dir re
  direcoesSimples(0, -100, 40, e, d);  CHECK(e == -1 && d == 1);      // gira esquerda
  direcoesSimples(39, 39, 40, e, d);   CHECK(e == 0 && d == 0);       // abaixo do limiar: parado
  direcoesSimples(0, 0, 40, e, d);     CHECK(e == 0 && d == 0);
  direcoesSimples(100, 100, 40, e, d); CHECK(e == 1 && d == 1);       // frente ganha do giro (como no v1)
  direcoesSimples(-50, -100, 40, e, d);CHECK(e == -1 && d == -1);     // re ganha do giro
  direcoesSimples(40, 0, 40, e, d);    CHECK(e == 1 && d == 1);       // exatamente no limiar: anda

  // ---- misturarDiferencial ----
  misturarDiferencial(1, 0, e, d);   CHECK_NEAR(e, 1, 1e-6); CHECK_NEAR(d, 1, 1e-6);
  misturarDiferencial(0, 1, e, d);   CHECK_NEAR(e, 1, 1e-6); CHECK_NEAR(d, -1, 1e-6);   // gira direita
  misturarDiferencial(0, -1, e, d);  CHECK_NEAR(e, -1, 1e-6); CHECK_NEAR(d, 1, 1e-6);
  misturarDiferencial(1, 1, e, d);   CHECK_NEAR(e, 1, 1e-6); CHECK_NEAR(d, 0, 1e-6);    // curva fechada direita
  misturarDiferencial(0.5f, 0.0f, e, d); CHECK_NEAR(e, 0.5, 1e-6); CHECK_NEAR(d, 0.5, 1e-6);
  misturarDiferencial(0, 0, e, d);   CHECK_NEAR(e, 0, 1e-6); CHECK_NEAR(d, 0, 1e-6);    // CENTRO = PARADO (o v1 andava aqui)
  for (float a = -1; a <= 1.001f; a += 0.25f)
    for (float s = -1; s <= 1.001f; s += 0.25f) {
      misturarDiferencial(a, s, e, d);
      CHECK(fabsf(e) <= 1.0001f && fabsf(d) <= 1.0001f);
    }

  // ---- aproximar e aproximarComFreio ----
  CHECK_NEAR(aproximar(0.0f, 1.0f, 0.1f), 0.1, 1e-6);
  CHECK_NEAR(aproximar(0.95f, 1.0f, 0.1f), 1.0, 1e-6);            // nao ultrapassa o alvo
  CHECK_NEAR(aproximarComFreio(0.0f, 1.0f, 0.10f, 0.34f), 0.10, 1e-6);   // acelerando: passo pequeno
  CHECK_NEAR(aproximarComFreio(1.0f, 0.0f, 0.10f, 0.34f), 0.66, 1e-6);   // soltou a alavanca: freia rapido
  CHECK_NEAR(aproximarComFreio(1.0f, -1.0f, 0.10f, 0.34f), 0.66, 1e-6);  // inversao tambem comeca freando
  CHECK_NEAR(aproximarComFreio(-0.5f, 0.0f, 0.10f, 0.34f), -0.16, 1e-6); // vale para re tambem
  CHECK_NEAR(aproximarComFreio(0.1f, 0.0f, 0.10f, 0.34f), 0.0, 1e-6);    // freio nao passa de zero
  {
    float v = 1.0f; int ciclos = 0;
    while (v != 0.0f && ciclos < 100) { v = aproximarComFreio(v, 0.0f, 0.10f, 0.34f); ciclos++; }
    CHECK(ciclos <= 3);                                           // 100% a 0% em no maximo 3 ciclos de 20 ms
    v = 0.0f; ciclos = 0;
    while (v != 1.0f && ciclos < 100) { v = aproximarComFreio(v, 1.0f, 0.10f, 0.34f); ciclos++; }
    CHECK(ciclos == 10);                                          // 0 a 100% em 10 ciclos (0,2 s)
  }

  // ---- velocidadeParaPwm ----
  CHECK(velocidadeParaPwm(0.0f, 80, 255) == 0);
  CHECK(velocidadeParaPwm(0.01f, 80, 255) == 0);
  CHECK(velocidadeParaPwm(1.0f, 80, 255) == 255);
  CHECK(velocidadeParaPwm(0.5f, 80, 255) == 168);
  CHECK(velocidadeParaPwm(2.0f, 80, 255) == 255);
  CHECK(velocidadeParaPwm(0.05f, 80, 255) >= 80);                 // nunca fica na "zona preguicosa"

  // ---- protocolo ----
  PacoteControle p;
  p.magic = PROTOCOLO_MAGIC; p.versao = PROTOCOLO_VERSAO; p.sequencia = 7;
  p.acelerador = 55; p.direcao = -30; p.flags = FLAG_MODO_MISTO;
  p.checksum = calcularChecksum(p);
  CHECK(sizeof(PacoteControle) == 7);
  CHECK(pacoteValido(p));
  PacoteControle q = p; q.acelerador = 56;                 CHECK(!pacoteValido(q));   // adulterado
  q = p; q.checksum ^= 0x01;                               CHECK(!pacoteValido(q));
  q = p; q.magic = 0xA7; q.checksum = calcularChecksum(q); CHECK(!pacoteValido(q));   // assinatura do ESP HAND CONTROL: ignorado
  q = p; q.versao = 1; q.checksum = calcularChecksum(q);   CHECK(!pacoteValido(q));
  q = p; q.acelerador = 101; q.checksum = calcularChecksum(q); CHECK(!pacoteValido(q));
  q = p; q.direcao = -101;   q.checksum = calcularChecksum(q); CHECK(!pacoteValido(q));
  q = p; q.acelerador = 100; q.direcao = -100; q.checksum = calcularChecksum(q); CHECK(pacoteValido(q));
  CHECK(FLAG_MODO_MISTO == 0x01);

  return resumo("logica");
}
