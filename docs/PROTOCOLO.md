# Protocolo de rádio do ODIR ESP-NOW 2.0

O controle envia ao carro um pacote fixo de **7 bytes** por ESP-NOW, no **canal 1**, 50 vezes por segundo (`PERIODO_ENVIO_MS = 20`). Definição em `firmware/comum/protocolo.h`.

## Formato do pacote

| Byte | Campo | Tipo | Valor |
|---:|---|---|---|
| 0 | `magic` | uint8 | `0xD1` (assinatura do ODIR ESP-NOW) |
| 1 | `versao` | uint8 | `2` |
| 2 | `sequencia` | uint8 | 0 a 255, sobe de 1 em 1 a cada envio e dá a volta |
| 3 | `acelerador` | int8 | -100 (ré a fundo) a +100 (frente a fundo) |
| 4 | `direcao` | int8 | -100 (esquerda) a +100 (direita) |
| 5 | `flags` | uint8 | bit 0 (`0x01`): 1 = modo MISTO, 0 = modo DIRECOES. Demais bits reservados (0) |
| 6 | `checksum` | uint8 | XOR dos bytes 0 a 5, com semente `0x5A` |

A estrutura é `packed` e há um `static_assert` garantindo que ela tem exatamente 7 bytes.

## Validação no carro

Um pacote só é aceito se **todas** estas condições forem verdadeiras:

1. tamanho recebido igual a 7 bytes;
2. `magic == 0xD1`;
3. `versao == 2`;
4. `checksum` confere;
5. `acelerador` e `direcao` entre -100 e +100.

Pacotes inválidos são contados (campo "ruins" do Monitor Serial) e ignorados.

## Perda de pacotes e failsafe

- O carro conta pacotes perdidos pela diferença entre números de sequência (campo "perdidos").
- Se nenhum pacote **válido** chega por `TIMEOUT_SINAL_MS` (400 ms), o carro entra em `SEM_SINAL`, zera o alvo dos motores e o freio rápido os para.
- A versão 1 usava um pacote cru de 3 bytes e esperava 1 s para parar; ela **não é compatível** com este protocolo (a `versao` e a assinatura são diferentes de propósito).

## Exemplo

Acelerador +60, direção 0, modo MISTO, sequência 5:

```text
D1 02 05 3C 00 01 B1
```

onde `B1` = `5A ^ D1 ^ 02 ^ 05 ^ 3C ^ 00 ^ 01`.

## Segurança

- A criptografia do ESP-NOW está **desligada** (`encrypt = false`). O checksum detecta corrupção, **não** impede falsificação.
- Padrão: o carro aceita pacotes de qualquer remetente que fale este protocolo. Para aceitar só um controle, defina `FILTRAR_REMETENTE 1` e `MAC_CONTROLE_PERMITIDO` em `config_carro.h`. Isso reduz o "sequestro" acidental em sala de aula, mas um MAC pode ser copiado: não é autenticação forte.
- Para ligar a criptografia (PMK e LMK), veja o desafio 1 do manual e a documentação do ESP-NOW da Espressif.

## Canal

Os dois dispositivos precisam estar no mesmo canal Wi-Fi. O código fixa o canal 1 (`CANAL_ESPNOW`). Se houver muita interferência de redes Wi-Fi no canal 1, experimente outro valor (1 a 11) **nos dois** sketches: a constante fica em `protocolo.h`, que é compartilhado. Lembre de sincronizar as pastas (`tools/sincronizar_comum.sh`).
