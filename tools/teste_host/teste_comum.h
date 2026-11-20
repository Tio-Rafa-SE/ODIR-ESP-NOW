// Mini-framework de teste (sem dependencias): CHECK conta falhas e imprime onde.
#pragma once
#include <stdio.h>
#include <math.h>

inline int g_total = 0, g_falhas = 0;

#define CHECK(cond)                                                                  \
  do {                                                                               \
    g_total++;                                                                       \
    if (!(cond)) { g_falhas++; printf("  FALHOU linha %d: %s\n", __LINE__, #cond); } \
  } while (0)

#define CHECK_NEAR(a, b, tol)                                                        \
  do {                                                                               \
    g_total++;                                                                       \
    double _a = (a), _b = (b);                                                       \
    if (fabs(_a - _b) > (tol)) {                                                     \
      g_falhas++;                                                                    \
      printf("  FALHOU linha %d: %s (=%g) != %s (=%g) tol %g\n", __LINE__, #a, _a, #b, _b, (double)(tol)); \
    }                                                                                \
  } while (0)

inline int resumo(const char *nome) {
  printf("[%s] %d verificacoes, %d falha(s)\n", nome, g_total, g_falhas);
  return g_falhas == 0 ? 0 : 1;
}
