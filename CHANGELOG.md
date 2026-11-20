# Changelog

## 2.0.0

Reescrita da versão 1 do ODIR ESP-NOW (código de Ujwal Nandanwar, adaptado por Rafael Oliveira), mantendo os mesmos pinos.

Novidades:
- Somente ESP32 clássico, com erro de compilação claro nas outras famílias.
- Uma única base de código para controle com 1 joystick ou com 2 joysticks.
- Compatível com Arduino-ESP32 2.x e 3.x (API de PWM e callback de recepção).
- Pacote de 7 bytes com assinatura, versão, sequência, flags e checksum.
- Dois modos de condução: DIRECOES (como no vídeo) e MISTO (proporcional), trocados pelo botão do joystick no controle.
- Centro do joystick medido ao ligar, zona morta com reescala linear, média de leituras.
- PWM nos pinos IN do L298N, com rampa de aceleração e freio mais rápido.
- Failsafe de 400 ms.
- Cinco sketches em ordem didática e testes no computador.
- Manual com interlúdios filosóficos, aviso sobre peças recicladas, diagnóstico e matriz de testes; passo a passo de montagem com conferências.

Pontos da v1 melhorados (por leitura do código, não testados em hardware):
- Modo "acelerador + direção" com joystick no centro podia mandar os dois motores para a frente.
- Botão alternava o modo a cada pacote recebido.
- Callback do rádio acionava motores e imprimia no Serial.
- Pacote sem validação; timeout de 1 s.
- MAC escrito direto no código.
- Compatível só com o core 3.x.

Correções de texto em relação ao material da v1:
- ESP-NOW não exige mestre e escravo.
- A criptografia é opcional e está desligada neste projeto.
- URL atual do pacote Arduino-ESP32: https://espressif.github.io/arduino-esp32/package_esp32_index.json

Validação: testado em hardware pelo autor (Rafael Oliveira), informado em 04/11/2026. Relatos linha a linha da matriz de testes são bem-vindos por issue.
