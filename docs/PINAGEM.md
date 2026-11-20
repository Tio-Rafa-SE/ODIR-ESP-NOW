# Pinagem do ODIR ESP-NOW 2.0

Somente **ESP32 clássico** (DevKit com módulo ESP-WROOM-32). Os pinos abaixo são os **mesmos do código original** do ODIR ESP-NOW; quem já montou o carro não refaz a fiação.

Fonte da verdade: `firmware/comum/config_carro.h` e `firmware/comum/config_controle.h`. Se este documento divergir do código, vale o código.

Status de validação: VALIDADO POR ANÁLISE (e por testes no computador com simulação). TESTADO EM HARDWARE pelo autor (Rafael Oliveira, informado em 04/11/2026).

## Carro (receptor)

| Função | GPIO | Destino | Observação |
|---|---:|---|---|
| Motor direito, sentido A | 16 | L298N IN1 | Livre só em módulo ESP-WROOM-32 (em WROVER é da PSRAM) |
| Motor direito, sentido B | 17 | L298N IN2 | Idem |
| Motor esquerdo, sentido A | 18 | L298N IN3 | |
| Motor esquerdo, sentido B | 19 | L298N IN4 | |
| LED de estado | 2 | LED azul da placa (se houver) | `-1` desliga. GPIO2 é pino de boot (strapping): nada deve forçá-lo em nível alto ao ligar |
| GND | GND | L298N GND | Terra comum, obrigatório |

- Direita e esquerda: vistas por quem olha o carro por trás, indo para a frente. **CONFIRME NA SUA MONTAGEM.**
- ENA e ENB do L298N: **jumpers instalados** (a velocidade vem do PWM nos pinos IN). **CONFIRME NA SUA MONTAGEM.**
- Alimentação do ESP32 do carro: **INFORMAÇÃO NÃO CONFIRMADA** (veja a seção 7.3 do manual).

## Controle (transmissor)

| Função | GPIO | Origem | Observação |
|---|---:|---|---|
| Acelerador (frente/trás) | 33 | VRy | ADC1, funciona com o rádio ligado |
| Direção (esquerda/direita) | 32 | VRx | ADC1, funciona com o rádio ligado |
| Botão (troca de modo) | 25 | SW | Digital, com o botão ligando o pino ao GND. GPIO25 é ADC2: **nunca** use como eixo |
| LED de estado | 2 | LED azul da placa (se houver) | `-1` desliga |
| Alimentação dos joysticks | 3V3 | +5V do módulo | **3V3, nunca 5V** |
| GND | GND | GND do módulo | |

### 1 joystick

| Módulo | ESP32 |
|---|---|
| +5V (no 3V3) | 3V3 |
| GND | GND |
| VRy | GPIO33 |
| VRx | GPIO32 |
| SW | GPIO25 |

### 2 joysticks

| Módulo | Fio | ESP32 |
|---|---|---|
| A (frente/trás) | VRy | GPIO33 |
| B (esquerda/direita) | VRx | GPIO32 |
| A ou B | SW | GPIO25 |
| A e B | +5V (no 3V3) e GND | 3V3 e GND |

Eixos que não são usados ficam **soltos**. Os pinos GPIO32 e GPIO33 nunca ficam no ar.

## Pinos a evitar neste projeto (ESP32 clássico)

| Grupo | GPIOs | Motivo |
|---|---|---|
| Strapping (influenciam o boot) | 0, 2, 5, 12, 15 | Nível errado na hora de ligar pode impedir o boot ou a gravação. O GPIO2 só é usado para o LED |
| ADC2 | 0, 2, 4, 12 a 15, 25 a 27 | Não funciona como entrada analógica com Wi-Fi/ESP-NOW ligado |
| Flash | 6 a 11 | Ligados à memória flash do módulo; não use |
| Somente entrada | 34 a 39 | Não têm saída nem pull-up interno |
| PSRAM (módulos WROVER) | 16 e 17 | Os motores deste projeto usam esses pinos; só use em WROOM |

CONFIRME NA SUA PLACA: o desenho exato dos pinos varia entre fabricantes de DevKit. Confira a serigrafia e o esquema da sua placa.

## Referência rápida de nomes no código

| Constante | Valor |
|---|---:|
| `PINO_MOTOR_DIR_IN1` | 16 |
| `PINO_MOTOR_DIR_IN2` | 17 |
| `PINO_MOTOR_ESQ_IN1` | 18 |
| `PINO_MOTOR_ESQ_IN2` | 19 |
| `PINO_ACELERADOR` | 33 |
| `PINO_DIRECAO` | 32 |
| `PINO_BOTAO` | 25 |
| `PINO_LED` | 2 |
| `CANAL_ESPNOW` | 1 |
