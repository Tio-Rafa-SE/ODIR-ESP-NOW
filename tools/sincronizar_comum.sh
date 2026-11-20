#!/usr/bin/env bash
# Copia os arquivos de firmware/comum/ para dentro de cada sketch que os usa.
# Motivo: a Arduino IDE so enxerga arquivos DENTRO da pasta do sketch.
# Uso:  tools/sincronizar_comum.sh           -> copia
#       tools/sincronizar_comum.sh --checar  -> so verifica se esta tudo igual (sai com erro se nao)
set -euo pipefail
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
COMUM="$RAIZ/firmware/comum"

declare -A USO=(
  [Passo1_Descobrir_MAC]="utilitarios.h"
  [Passo2_Teste_Joystick]="config_controle.h logica.h utilitarios.h"
  [Passo3_Teste_Motores]="config_carro.h logica.h motores.h utilitarios.h"
  [Passo4_Carro_Receptor]="config_carro.h protocolo.h logica.h motores.h utilitarios.h"
  [Passo5_Controle_Transmissor]="config_controle.h protocolo.h logica.h utilitarios.h"
)

erro=0
for sketch in "${!USO[@]}"; do
  for arq in ${USO[$sketch]}; do
    destino="$RAIZ/firmware/$sketch/$arq"
    if [[ "${1:-}" == "--checar" ]]; then
      if ! cmp -s "$COMUM/$arq" "$destino"; then echo "DIFERENTE: $sketch/$arq"; erro=1; fi
    else
      cp "$COMUM/$arq" "$destino"
    fi
  done
done
if [[ "${1:-}" == "--checar" ]]; then
  [[ $erro -eq 0 ]] && echo "OK: todas as copias estao identicas a firmware/comum/." || exit 1
else
  echo "Sincronizado."
fi
