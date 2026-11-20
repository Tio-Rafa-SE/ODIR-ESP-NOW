# Passo a passo da montagem do ODIR ESP-NOW

Guia de montagem do carro e do controle, baseado nos slides do projeto original (kit MonTTudo, Mamute Eletrônica), com **pontos de conferência** em cada etapa e **alternativas recicladas**.

> **Sem o kit MonTTudo? Use peças recicladas.** Papelão grosso, acrílico ou MDF de sucata para o chassi; tampinhas ou CDs para as rodas; motores de brinquedo quebrado; joystick de videogame quebrado. A mecânica é flexível. A eletrônica (ESP32, L298N, GND comum, 3V3 nos joysticks) não é. Veja os cuidados com baterias de lítio no manual.

**O que este guia NÃO traz:** nomes oficiais das peças de MDF, quantidades de parafusos e medidas. Os slides originais mostram as peças em fotos, sem lista numerada, e o autor deste pacote não vai inventar nomes nem números. Use as fotos do vídeo e o encarte do seu kit como referência. Onde falta informação, está escrito **CONFIRME NO SEU KIT**.

Status: este guia foi escrito a partir dos slides e da análise do projeto. Não foi validado montando o carro. Se você montar, melhore o guia e mande de volta.

---

## Antes de começar

**Ferramentas e materiais (pelo que aparece nos slides):** chave de fenda, ferro de solda com estanho (os fios dos motores aparecem sendo soldados), jumpers fêmea-fêmea, fita ou presilhas para organizar fios. **CONFIRME NO SEU KIT** o que acompanha cada item.

**Organize a mesa.** Separe as peças em dois grupos: carro e controle. Fotografe tudo antes de começar; ajuda muito na hora de montar de novo.

**Regra de ouro:** só feche a caixa e prenda tudo depois de testar a eletrônica fora dela (Passos 1 a 5 do manual). Montar, fechar, descobrir um fio solto e abrir de novo é a história mais antiga da robótica.

---

## Parte 1: o carro

O carro tem um chassi octogonal de MDF perfurado, dois motores amarelos com caixa de redução, duas rodas grandes, um apoio na frente (roda boba), o suporte de baterias, a ponte H L298N e a ESP32.

### Etapa 1: o chassi e as paredes

Nos slides, o chassi começa com a placa octogonal e as peças laterais encaixadas na base.

- Encaixe as peças de MDF conforme as fotos do passo a passo original.
- Se precisar parafusar, aperte devagar: MDF racha.

**Conferência 1:** o chassi está firme, sem peças tortas, e as paredes não balançam.

*Reciclado:* uma base rígida (papelão grosso, acrílico, MDF de móvel velho) com furos para fixar os módulos. Dica: perfure em grade e amarre os módulos com abraçadeiras.

### Etapa 2: o apoio da frente (roda boba)

O carro tem duas rodas motorizadas atrás e um apoio de deslizamento na frente.

- Monte o suporte e o apoio como nas fotos do original.
- O apoio precisa ficar na altura certa para o chassi ficar nivelado.

**Conferência 2:** o carro apoiado sobre as rodas e o apoio fica nivelado; empurre com a mão: o apoio gira livremente.

*Reciclado:* tampinha lisa, bolinha de desodorante roll-on, pé de móvel de plástico.

### Etapa 3: os motores e os fios

Os slides mostram os dois motores amarelos, com fios soldados em cada um.

- Se os motores vieram sem fios, **solde** um fio em cada terminal, com ferro de solda e estanho. Faça isso com ventilação, e peça ajuda a um adulto se for iniciante.
- Prenda cada motor no chassi.

**Conferência 3:** os fios soldados não saem com um puxão leve. Os dois motores estão alinhados e as rodas vão ficar paralelas.

*Reciclado:* motores de brinquedos quebrados. **Meça antes de ligar:** descubra a tensão e o consumo (o que está escrito no motor, ou teste com tensão baixa e observe).

### Etapa 4: suporte de baterias, L298N e ESP32

Nos slides, o suporte de baterias é preso na parte de baixo do chassi, e a ponte H L298N (módulo vermelho) e a ESP32 ficam na parte de cima, ligados com fios coloridos.

- Fixe o suporte de baterias, o L298N e a ESP32.
- Ligue conforme a figura `img/ligacao_carro.svg` e a tabela da seção 7.1 do manual:

| ESP32 | L298N |
|---|---|
| GPIO16 | IN1 |
| GPIO17 | IN2 |
| GPIO18 | IN3 |
| GPIO19 | IN4 |
| GND | GND |

- Motor direito nos terminais OUT1/OUT2 e motor esquerdo em OUT3/OUT4 (direita e esquerda vistas por trás, olhando para a frente: **CONFIRME NA SUA MONTAGEM**).
- Jumpers de ENA e ENB **instalados**.
- Bateria ao L298N. **A alimentação do ESP32 não está confirmada** neste pacote: veja a seção 7.3 do manual e escolha a opção que a sua montagem usa.

**Conferência 4 (antes de ligar):** com a bateria **desconectada**, confira fio a fio com a tabela. Nenhum fio de motor encostado em fio de sinal. GND do ESP32 e do L298N ligados.

*Reciclado:* qualquer suporte de pilhas, desde que você saiba a tensão total e que ela esteja na faixa do seu módulo L298N (**CONFIRME NO DATASHEET**).

### Etapa 5: rodas e conferência do carro

- Encaixe as rodas nos eixos.
- Siga os **Passos 1 e 3** do manual (descobrir o MAC e testar os motores) **com o carro levantado**.

**Conferência 5:** cada roda gira no sentido certo no Passo 3, e as duas giram juntas "para frente" quando o Passo 3 pede.

### Etapa 6: fechar o carro (só depois de testar)

Só depois do Passo 3 passar:

- Prenda os fios com fita ou presilhas, longe das rodas.
- Feche o chassi conforme as fotos originais.

---

## Parte 2: o controle

O controle é uma caixinha de MDF que guarda a ESP32 e o joystick. Nos slides, o joystick fica preso em uma peça com furo e entra por cima da caixa.

### Entenda o joystick

O módulo joystick é **dois potenciômetros** (um para o eixo X e outro para o eixo Y) **mais um botão** (o eixo Z). Os potenciômetros devolvem uma tensão proporcional à posição da alavanca; o ESP32 mede essa tensão.

**Conferência de segurança:** o módulo vem marcado "+5V", mas aqui **vai no 3V3** da ESP32.

### Etapa 7: o joystick na peça de MDF

- Fixe o módulo joystick à peça de MDF, como mostram os slides (o módulo é preso em camadas de MDF por parafusos).
- A peça com o furo deixa a alavanca passando por cima.

**Conferência 7:** a alavanca se move livremente em todas as direções e o botão (apertar a alavanca) funciona.

### Etapa 8: ligar o joystick à ESP32 com jumpers

Os slides mostram jumpers **fêmea-fêmea** ligando o módulo à ESP32.

Escolha **uma** das duas montagens (o código é o mesmo):

- **1 joystick** (todas as direções): `img/ligacao_controle_1_joystick.svg`.
- **2 joysticks** (um para frente e trás, outro para esquerda e direita): `img/ligacao_controle_2_joysticks.svg`.

Tabela resumida (1 joystick): 3V3, GND, VRy no GPIO33, VRx no GPIO32, SW no GPIO25. Para 2 joysticks: A (VRy no GPIO33) e B (VRx no GPIO32), SW de um deles no GPIO25.

**Conferência 8:** com o controle ainda **aberto**, rode o **Passo 2** do manual. Sem tocar, ADC perto de 2048. Empurre a alavanca: percentual positivo para frente e para a direita. Aperte: "apertado". Se um eixo grudar em 0 ou 4095, o sketch avisa ALERTA.

*Reciclado:* joystick de controle de videogame quebrado serve. Por dentro tem dois potenciômetros e um botão. Descubra os terminais com um multímetro (resistência entre os terminais de cada potenciômetro varia ao mexer a alavanca). Alimentação em 3V3.

### Etapa 9: fonte de alimentação e caixa

Os slides mostram a ESP32 do controle dentro da caixa de MDF, com o joystick por cima. A alimentação do controle depende do kit (cabo USB com power bank, bateria, etc.): **CONFIRME NO SEU KIT**.

- Só feche a caixa **depois** do Passo 5 funcionar (carro e controle conversando).

**Conferência 9:** com a caixa fechada, o joystick continua livre, o cabo de energia está seguro e a ESP32 está acessível o bastante para você reiniciar ou regravar.

---

## Parte 3: primeira volta

1. Rode o **Passo 4** no carro e o **Passo 5** no controle (veja o manual, seção 8).
2. Primeiro teste com `LIMITE_VELOCIDADE_PCT = 60`, em um espaço livre.
3. Teste a perda de sinal: desligue o controle com o carro andando. Ele deve parar em menos de meio segundo.
4. Preencha a **matriz de testes** do manual (seção 12) com o resultado real. Se tirar fotos, melhor ainda.

Está funcionando? Compartilhe: este guia é um rascunho melhorado e quer ser melhorado de novo.

---

## Pontos que ficam para você confirmar

| Ponto | Situação |
|---|---|
| Nomes oficiais das peças de MDF e número de parafusos | CONFIRME NO SEU KIT |
| Como o ESP32 do carro é alimentado | INFORMAÇÃO NÃO CONFIRMADA (manual, seção 7.3) |
| Jumpers de ENA e ENB no L298N | CONFIRME NA SUA MONTAGEM |
| Motor direito e esquerdo, vistos por trás | CONFIRME NA SUA MONTAGEM |
| Alimentação do controle | CONFIRME NO SEU KIT |
| Tipo e tensão das baterias | CONFIRME NO SEU KIT |
