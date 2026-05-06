#include <esp_now.h>
#include <WiFi.h>

// DIRECCIÓN DEL MAESTRO
uint8_t direccionMaestro[] = {0xD4, 0xE9, 0xF4, 0x65, 0xC4, 0xF4};

typedef struct struct_mensaje {
  int id_placa;
  int gas;
  int humedad_tierra;
} struct_mensaje;

struct_mensaje misDatos;
esp_now_peer_info_t infoMaestro;

#define PIN_GAS 34
#define PIN_TIERRA 35

void AlEnviarDatos(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  Serial.print("\r\nEstado: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ ENTREGA EXITOSA" : "❌ FALLO EN LA ENTREGA");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_send_cb(AlEnviarDatos);
  memcpy(infoMaestro.peer_addr, direccionMaestro, 6);
  infoMaestro.channel = 0;  
  infoMaestro.encrypt = false; 
  esp_now_add_peer(&infoMaestro);
}

void loop() {
  misDatos.id_placa = 2; 
  
  // 1. LECTURA Y VALIDACIÓN DEL GAS
  int lecturaBrutaGas = analogRead(PIN_GAS);
  if (lecturaBrutaGas < 200 || lecturaBrutaGas >= 4090) {
    misDatos.gas = -1; // Código de error: Desconectado
  } else {
    misDatos.gas = lecturaBrutaGas;
  }

  // 2. LECTURA Y VALIDACIÓN DE TIERRA
  int lecturaBrutaTierra = analogRead(PIN_TIERRA);
  if (lecturaBrutaTierra < 200 || lecturaBrutaTierra >= 4090) {
    misDatos.humedad_tierra = -1; // Código de error: Desconectado
  } else {
    misDatos.humedad_tierra = map(lecturaBrutaTierra, 4095, 0, 0, 100); 
    misDatos.humedad_tierra = constrain(misDatos.humedad_tierra, 0, 100); // Limita entre 0 y 100
  }

  esp_now_send(direccionMaestro, (uint8_t *) &misDatos, sizeof(misDatos));
  delay(2000);
}