#include <TFT_eSPI.h>
#include <DHT.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <esp_now.h>
#include <esp_wifi.h>

TFT_eSPI tft = TFT_eSPI();

// ==========================================
// 1. CONFIGURACIÓN DE NUBE (UNIVERSIDAD - GRUPO 2)
// ==========================================
const char* serverUrl = "https://grupo2top3.comunidadingenieria.cl/ingesta.php";

bool wifiActivado = false;

// ==========================================
// 2. CONFIGURACIÓN DE SENSORES Y HARDWARE
// ==========================================
#define DATA_PIN 27
#define DHTTYPE DHT22
DHT dht(DATA_PIN, DHTTYPE);

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

int modoActual = 0;

// --- CRONÓMETROS INDEPENDIENTES ---
unsigned long ultimoTiempoPantalla = 0;
unsigned long ultimoTiempoNube = 0;

// --- LÓGICA TÁCTIL (5 SEGUNDOS) ---
unsigned long tiempoInicioToque = 0;
bool estaTocando = false;
int ultimoPixelX = 0;
int ultimoPixelY = 0;

// Variables Globales de Nube
float ultimaTemp = -100.0;
float ultimaHum = -100.0;
int ultimoGas = -1;

// --- TOGGLES DEL NODO REMOTO ---
bool habilitarGasRemoto = true;
bool habilitarTierraRemoto = true;

// --- CACHE GAS LOCAL (CICLO DE RADIO PARA LIBERAR ADC2) ---
unsigned long ultimoTiempoCicloRadio = 0;
const unsigned long INTERVALO_CICLO_RADIO = 5000;
int gasLocalCacheado = -1;
bool radioCicladaUnaVez = false;

// ==========================================
// ESTRUCTURA ESP-NOW (NODO REMOTO)
// ==========================================
typedef struct struct_mensaje {
  int id_placa;
  int gas;
  int humedad_tierra;
} struct_mensaje;

struct_mensaje datosRecibidos;
int remotoGas = -1;
int remotoTierra = -1;
unsigned long ultimoTiempoRemoto = 0;

void AlRecibirDatos(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&datosRecibidos, incomingData, sizeof(datosRecibidos));
  remotoGas = datosRecibidos.gas;
  remotoTierra = datosRecibidos.humedad_tierra;
  ultimoTiempoRemoto = millis();
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);

  dht.begin();

  WiFiManager wm;
  wm.setConnectTimeout(10);
  if(wm.autoConnect("ESP32_Smart_Monitor")) {
      wifiActivado = true;
      Serial.println("WiFi Conectado Automáticamente");
  }

  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(AlRecibirDatos);
    Serial.println("ESP-NOW listo (escuchando Nodo 2)");
  } else {
    Serial.println("Error iniciando ESP-NOW");
  }

  dibujarInterfazBasica();
  dibujarGuiaCables();
}

void loop() {
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    ultimoPixelX = map(p.x, 300, 3800, 0, 320);
    ultimoPixelY = map(p.y, 300, 3800, 0, 240);

    if (!estaTocando) {
      tiempoInicioToque = millis();
      estaTocando = true;
    }

    if (millis() - tiempoInicioToque > 5000 && ultimoPixelX > 160 && ultimoPixelY >= 180) {
      activarModoConfiguracion();
      estaTocando = false;
    }
  } else {
    if (estaTocando) {
      unsigned long duracion = millis() - tiempoInicioToque;

      if (duracion < 5000) {
        bool tomamosToggle = false;

        // Toggles del Nodo Remoto (solo en modo 2, zona y=110..145)
        if (modoActual == 2 && ultimoPixelY >= 110 && ultimoPixelY <= 145) {
          if (ultimoPixelX < 160) {
            habilitarGasRemoto = !habilitarGasRemoto;
            if (!habilitarGasRemoto) remotoGas = -1;
          } else {
            habilitarTierraRemoto = !habilitarTierraRemoto;
            if (!habilitarTierraRemoto) remotoTierra = -1;
          }
          tomamosToggle = true;
        }

        if (!tomamosToggle) {
          if (ultimoPixelX < 160) {
            modoActual = (modoActual == 2) ? 0 : modoActual + 1;
          } else {
            wifiActivado = !wifiActivado;
            if(wifiActivado) WiFi.begin(); else WiFi.disconnect();
          }
        }

        tft.fillScreen(TFT_BLACK);
        dibujarInterfazBasica();
        dibujarGuiaCables();
      }
      estaTocando = false;
    }
  }

  if (millis() - ultimoTiempoPantalla > 2000) {
    ultimoTiempoPantalla = millis();
    tft.fillRect(10, 60, 300, 85, TFT_BLACK);

    if (modoActual == 0) {
      mostrarDatosDHT();
    } else if (modoActual == 1) {
      mostrarDatosGas();
    } else {
      mostrarDatosRemotos();
    }
  }

  if (millis() - ultimoTiempoNube > 15000) {
    ultimoTiempoNube = millis();
    if (wifiActivado && WiFi.status() == WL_CONNECTED) {
      enviarDatosNube();
    }
  }
}

// ==========================================
// 3. PORTAL CAUTIVO
// ==========================================
void activarModoConfiguracion() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawCentreString("MODO CONFIGURACION", 160, 40, 4);
  tft.drawCentreString("Conectate con tu celular al WiFi:", 160, 90, 2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString("ESP32_Setup", 160, 120, 4);

  WiFiManager wm;
  if (!wm.startConfigPortal("ESP32_Setup")) {
    ESP.restart();
  }

  wifiActivado = true;
  tft.fillScreen(TFT_BLACK);
  dibujarInterfazBasica();
  dibujarGuiaCables();
}

// ==========================================
// ENVÍO HTTP POST
// ==========================================
void enviarDatosNube() {
  // Envío global: todos los sensores válidos, sin importar la pestaña activa.
  if (ultimaTemp != -100.0) {
    enviarPost("DHT22_Temp", ultimaTemp);
    delay(500);
    enviarPost("DHT22_Hum", ultimaHum);
    delay(500);
  }

  if (ultimoGas != -1) {
    enviarPost("Local_MQ135", ultimoGas);
    delay(500);
  }

  if (millis() - ultimoTiempoRemoto < 10000) {
    if (habilitarGasRemoto && remotoGas != -1) {
      enviarPost("Remoto_MQ135", remotoGas);
      delay(500);
    }
    if (habilitarTierraRemoto && remotoTierra != -1) {
      enviarPost("Remoto_Humedad_Tierra", remotoTierra);
      delay(500);
    }
  }
}

void enviarPost(String nombreDispositivo, float valor) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postData = "dispositivo=" + nombreDispositivo + "&valor=" + String(valor);
  int httpCode = http.POST(postData);

  if (httpCode > 0) {
    Serial.printf("[POST OK] %s -> %.2f (HTTP %d) - Envio Exitoso\n", nombreDispositivo.c_str(), valor, httpCode);
  } else {
    Serial.printf("[POST ERROR] Fallo envio de %s: %s\n", nombreDispositivo.c_str(), http.errorToString(httpCode).c_str());
  }
  http.end();
}

// ==========================================
// 4. FUNCIONES DE INTERFAZ GRÁFICA
// ==========================================
void dibujarInterfazBasica() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  String titulo = "TEMPERATURA";
  if (modoActual == 1) titulo = "GAS AMBIENTAL";
  if (modoActual == 2) titulo = "NODO REMOTO";

  tft.setTextSize(1);
  tft.drawCentreString(titulo, 160, 10, 4);
  tft.drawFastHLine(10, 40, 300, TFT_BLUE);

  tft.fillRoundRect(10, 190, 140, 40, 5, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawCentreString("SENSOR", 80, 200, 1);

  uint16_t colorWifiBtn = wifiActivado ? TFT_DARKGREEN : TFT_MAROON;
  tft.fillRoundRect(170, 190, 140, 40, 5, colorWifiBtn);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(wifiActivado ? "WIFI: ON" : "WIFI: OFF", 240, 200, 1);
}

void dibujarGuiaCables() {
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  if (modoActual == 0) {
    tft.drawCentreString("GUIA DHT22 (Sensor Blanco):", 160, 145, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawCentreString("ROJO->VCC | NEGRO->GND | AMARILLO->DAT", 160, 165, 2);
  } else if (modoActual == 1) {
    tft.drawCentreString("GUIA GAS (Sensor Azul):", 160, 145, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawCentreString("ROJO->VCC | NEGRO->GND | AMARILLO->AO", 160, 165, 2);
  } else {
    tft.drawCentreString("NODO 2 (Protoboard):", 160, 145, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawCentreString("GAS:34 | TIERRA:35 | VCC:3.3V | GND:GND", 160, 165, 2);
  }
}

// Toggles dibujados dentro de la zona limpiada (y=110..140) y redibujados
// cada 2 s por mostrarDatosRemotos.
void dibujarTogglesRemoto() {
  uint16_t cGas    = habilitarGasRemoto    ? TFT_DARKGREEN : TFT_MAROON;
  uint16_t cTierra = habilitarTierraRemoto ? TFT_DARKGREEN : TFT_MAROON;

  tft.fillRoundRect(15, 113, 135, 28, 5, cGas);
  tft.fillRoundRect(170, 113, 135, 28, 5, cTierra);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawCentreString(habilitarGasRemoto    ? "GAS REM: ON"  : "GAS REM: OFF",  82,  120, 2);
  tft.drawCentreString(habilitarTierraRemoto ? "TIERRA: ON"   : "TIERRA: OFF",   237, 120, 2);
}

void mostrarDatosDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  tft.setTextSize(1);

  if (isnan(h) || isnan(t)) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString("DESCONECTADO", 160, 80, 4);
    ultimaTemp = -100.0;
  } else {
    ultimaTemp = t;
    ultimaHum = h;
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(30, 65);  tft.print("Temp: " + String(t, 1) + " C");
    tft.setCursor(30, 100); tft.print("Hum:  " + String(h, 1) + " %");
    Serial.print("DATA,T,"); Serial.print(t); Serial.print(",H,"); Serial.println(h);
  }
}

// --- Apaga radio + ESP-NOW, lee ADC2 limpio, reactiva todo ---
// Solo se ejecuta cada INTERVALO_CICLO_RADIO ms para no asfixiar la red.
int leerGasLocalSinRadio() {
  if (radioCicladaUnaVez && (millis() - ultimoTiempoCicloRadio < INTERVALO_CICLO_RADIO)) {
    return gasLocalCacheado;
  }

  // 1. Apagar ESP-NOW (suelta callbacks)
  esp_now_deinit();

  // 2. Apagar la radio WiFi (libera el lock del ADC2)
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  delay(80);

  // 3. Lectura ADC2 con la radio apagada (promedio de 5)
  long suma = 0;
  for (int i = 0; i < 5; i++) {
    suma += analogRead(DATA_PIN);
    delay(3);
  }
  int valor = suma / 5;

  // 4. Reactivar radio + ESP-NOW
  esp_wifi_start();
  WiFi.mode(WIFI_STA);
  if (wifiActivado) {
    WiFi.begin();
  }
  delay(30);
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(AlRecibirDatos);
  }

  gasLocalCacheado = valor;
  ultimoTiempoCicloRadio = millis();
  radioCicladaUnaVez = true;
  return valor;
}

void mostrarDatosGas() {
  int nivelGas = leerGasLocalSinRadio();
  tft.setTextSize(1);

  if (nivelGas < 200 || nivelGas >= 4090) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("DESCONECTADO", 160, 80, 4);
      tft.fillRect(200, 60, 70, 30, TFT_BLACK);
      ultimoGas = -1;
  } else {
      ultimoGas = nivelGas;
      uint16_t colorGas = TFT_GREEN;
      if (nivelGas > 1500) colorGas = TFT_RED;
      else if (nivelGas > 950) colorGas = TFT_ORANGE;

      tft.setTextColor(colorGas, TFT_BLACK);
      tft.setCursor(20, 75);
      tft.print("Nivel: " + String(nivelGas) + "      ");

      dibujarBarrasLaterales(nivelGas, 65);

      Serial.print("DATA,G,"); Serial.println(nivelGas);
  }
}

// ==========================================
// TERCERA PANTALLA (NODO REMOTO)
// ==========================================
void mostrarDatosRemotos() {
  tft.setTextSize(1);
  bool sinSenal = (millis() - ultimoTiempoRemoto > 10000);

  // 1. Gas Remoto
  tft.setCursor(20, 65);
  if (!habilitarGasRemoto) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("Gas Remoto: DESACTIVADO  ");
  } else if (sinSenal || remotoGas == -1) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Gas Remoto: SIN SENAL    ");
  } else {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print("Gas Remoto: " + String(remotoGas) + "      ");
    dibujarBarrasLaterales(remotoGas, 60);
  }

  // 2. Tierra Remota
  tft.setCursor(20, 90);
  if (!habilitarTierraRemoto) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("Tierra:     DESACTIVADO  ");
  } else if (sinSenal || remotoTierra == -1) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("Tierra:     SIN SENAL    ");
  } else {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print("Tierra:     " + String(remotoTierra) + " %    ");
  }

  // Toggles redibujados (viven dentro de la zona limpiada)
  dibujarTogglesRemoto();
}

void dibujarBarrasLaterales(int valor, int yPos) {
  int xStart = 200;
  tft.fillRect(xStart, yPos + 15, 20, 10, (valor > 100) ? TFT_GREEN : TFT_BLACK);
  tft.fillRect(xStart + 25, yPos + 7, 20, 18, (valor > 950) ? TFT_YELLOW : TFT_BLACK);
  tft.fillRect(xStart + 50, yPos, 20, 25, (valor > 1500) ? TFT_RED : TFT_BLACK);
}
