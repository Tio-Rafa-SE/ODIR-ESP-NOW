// ============================================================================
//  ODIR ESP-NOW 2.0  -  Passo 1: descobrir o MAC do CARRO
//  Prof. Rafael Oliveira (Tio Rafa) - Robotica Paula Souza
//  Mecatronica Jovem, edicao 19 - "ODIR com ESP32 e ESP-NOW"
//
//  Grave este sketch no ESP32 que vai ficar NO CARRO. Ele imprime o endereco
//  fisico (MAC) da placa, ja no formato para colar no config_controle.h.
//
//  Por que isso e necessario? No ESP-NOW o controle manda o pacote para um
//  endereco MAC especifico - e como o "CEP" do carro. Cada placa tem o seu.
//
//  Resultado esperado (Monitor Serial, 115200 baud), repetido a cada 2 s:
//     MAC (texto)  : E8:DB:84:0F:AD:B4          <- (o seu sera diferente)
//     MAC (codigo) : {0xE8, 0xDB, 0x84, 0x0F, 0xAD, 0xB4};
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include "utilitarios.h"

static void imprimirMac() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("MAC (texto)  : %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("MAC (codigo) : {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X};\n\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void setup() {
  iniciarSerial();
  // O Wi-Fi precisa estar ligado (modo estacao) para o MAC ser lido direito.
  WiFi.mode(WIFI_STA);
  delay(200);
  Serial.println();
  Serial.println("=== ODIR ESP-NOW 2.0 - Passo 1: MAC do carro ===");
}

void loop() {
  // Repete a cada 2 s: se voce abriu o Monitor Serial atrasado, nao perde nada.
  imprimirMac();
  delay(2000);
}
