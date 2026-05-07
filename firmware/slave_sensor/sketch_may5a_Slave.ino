#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h> // LIBRERÍA NECESARIA PARA CAMBIAR DE CANAL

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

// --- VARIABLES PARA AUTO-RECUPERACIÓN ---
int fallosConsecutivos = 0;
int canalActual = 1; // Empezamos la búsqueda en el canal 1

// Notarás que volvemos a usar la estructura moderna de ESP-NOW V3
void AlEnviarDatos(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.print("✅ ÉXITO en canal: ");
    Serial.println(canalActual);
    fallosConsecutivos = 0; // Si hay éxito, reseteamos el contador
  } else {
    Serial.println("❌ FALLO. El Maestro no responde.");
    fallosConsecutivos++; // Sumamos un fallo
  }
}

void setup() {
  Serial.begin(115200);
  
  // Iniciamos WiFi
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_send_cb(AlEnviarDatos);

  // Configuramos al maestro en el canal "0" (significa que seguirá el canal de la placa)
  memcpy(infoMaestro.peer_addr, direccionMaestro, 6);
  infoMaestro.channel = 0;  
  infoMaestro.encrypt = false; 
  esp_now_add_peer(&infoMaestro);
}

void loop() {
  // --- ALGORITMO DE CHANNEL HOPPING (BUSCADOR DE ROUTER) ---
  if (fallosConsecutivos >= 3) {
    canalActual++; // Saltamos al siguiente canal
    if (canalActual > 13) {
      canalActual = 1; // Si pasamos el 13, volvemos al 1
    }
    
    Serial.print("📡 Buscando al maestro en el canal: ");
    Serial.println(canalActual);
    
    // Forzamos a la antena de la placa a cambiar de frecuencia
    esp_wifi_set_channel(canalActual, WIFI_SECOND_CHAN_NONE);
    
    // Le damos 3 intentos en este nuevo canal antes de volver a saltar
    fallosConsecutivos = 0; 
    delay(500); // Pausa para que el radiofrecuencia se estabilice
  }

  // --- LECTURA DE SENSORES (INTACTA) ---
  misDatos.id_placa = 2; 
  
  int lecturaBrutaGas = analogRead(PIN_GAS);
  if (lecturaBrutaGas < 100 || lecturaBrutaGas >= 4090) {
    misDatos.gas = -1; 
  } else {
    misDatos.gas = lecturaBrutaGas;
  }

  int lecturaBrutaTierra = analogRead(PIN_TIERRA);
  if (lecturaBrutaTierra < 100 || lecturaBrutaTierra >= 4090) {
    misDatos.humedad_tierra = -1; 
  } else {
    misDatos.humedad_tierra = map(lecturaBrutaTierra, 4095, 0, 0, 100); 
    misDatos.humedad_tierra = constrain(misDatos.humedad_tierra, 0, 100); 
  }

  // Enviamos los datos
  esp_now_send(direccionMaestro, (uint8_t *) &misDatos, sizeof(misDatos));
  
  delay(2000);
}