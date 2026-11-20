#!/usr/bin/env bash
# Compila e roda os testes no computador (g++), para as 2 versoes do core:
#   core : Arduino-ESP32 2.x | 3.x   (APIs de LEDC e callback do ESP-NOW diferentes)
# Placa: so ESP32 classico (e o escopo do projeto). Cada combinacao compila TODOS os 5 sketches,
# com -Wall -Wextra. Tambem confirma que outra familia (ex.: ESP32-S3) e RECUSADA no compilador.
#
# IMPORTANTE: isto valida logica e sintaxe contra STUBS que imitam o Arduino-ESP32.
# NAO substitui compilar com o core real nem testar em hardware.
set -uo pipefail
AQUI="$(cd "$(dirname "$0")" && pwd)"
RAIZ="$(cd "$AQUI/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

"$RAIZ/tools/sincronizar_comum.sh" --checar || exit 1

# Copia o firmware e planta um MAC de teste (o controle se recusa a iniciar com MAC zerado).
cp -r "$RAIZ/firmware" "$TMP/firmware"
sed -i 's/MAC_CARRO\[6\] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}/MAC_CARRO[6] = {0xE8, 0xDB, 0x84, 0x0F, 0xAD, 0xB4}/' \
  "$TMP/firmware/Passo5_Controle_Transmissor/config_controle.h"
grep -q "0xE8, 0xDB" "$TMP/firmware/Passo5_Controle_Transmissor/config_controle.h" || { echo "falha ao plantar MAC de teste"; exit 1; }

FALHAS=0
CXX="g++ -std=gnu++17 -Wall -Wextra -Wno-unused-parameter"

echo "=== logica pura (sem stubs) ==="
$CXX -I"$TMP/firmware/comum" -I"$AQUI" "$AQUI/teste_logica.cpp" -o "$TMP/t_logica" -lm && "$TMP/t_logica" || FALHAS=$((FALHAS+1))

for CORE in 2 3; do
  echo
  echo "=== placa=ESP32 classico  core=$CORE.x ==="
  DEFS="-DCONFIG_IDF_TARGET_ESP32 -DESP_ARDUINO_VERSION_MAJOR=$CORE"
  INC="-I$AQUI/stubs -I$AQUI"

  S4="$TMP/firmware/Passo4_Carro_Receptor/Passo4_Carro_Receptor.ino"
  S5="$TMP/firmware/Passo5_Controle_Transmissor/Passo5_Controle_Transmissor.ino"
  $CXX $DEFS $INC -I"$(dirname "$S4")" -DTESTE_SKETCH="\"$S4\"" -x c++ "$AQUI/teste_carro.cpp" -o "$TMP/t_carro" -lm \
    && "$TMP/t_carro" || FALHAS=$((FALHAS+1))
  $CXX $DEFS $INC -I"$(dirname "$S5")" -DTESTE_SKETCH="\"$S5\"" -x c++ "$AQUI/teste_controle.cpp" -o "$TMP/t_controle" -lm \
    && "$TMP/t_controle" || FALHAS=$((FALHAS+1))

  # Fumaca: Passo 1, 2 e 3 compilam e rodam setup()+loop()
  for P in Passo1_Descobrir_MAC Passo2_Teste_Joystick Passo3_Teste_Motores; do
    S="$TMP/firmware/$P/$P.ino"
    $CXX $DEFS $INC -I"$(dirname "$S")" -DTESTE_SKETCH="\"$S\"" -x c++ "$AQUI/teste_fumaca.cpp" -o "$TMP/t_$P" -lm \
      && "$TMP/t_$P" && echo "[$P] compilou e rodou setup()+loop()" || { echo "[$P] FALHOU"; FALHAS=$((FALHAS+1)); }
  done
done

echo
echo "=== familia errada (ESP32-S3) tem de ser RECUSADA ==="
S="$TMP/firmware/Passo4_Carro_Receptor/Passo4_Carro_Receptor.ino"
SAIDA="$($CXX -DCONFIG_IDF_TARGET_ESP32S3 -DESP_ARDUINO_VERSION_MAJOR=3 -I"$AQUI/stubs" -I"$AQUI" -I"$(dirname "$S")" \
  -DTESTE_SKETCH="\"$S\"" -x c++ "$AQUI/teste_fumaca.cpp" -o "$TMP/t_s3" -lm 2>&1)"
if [[ $? -ne 0 ]] && grep -q "so para o ESP32 classico" <<<"$SAIDA"; then
  echo "[ok] ESP32-S3 recusado com mensagem clara"
else
  echo "[FALHOU] ESP32-S3 deveria ser recusado"; FALHAS=$((FALHAS+1))
fi

echo
if [[ $FALHAS -eq 0 ]]; then echo "TUDO OK: 2 versoes do core (ESP32 classico) + logica + recusa de familia errada."; else echo "FALHAS: $FALHAS"; exit 1; fi
