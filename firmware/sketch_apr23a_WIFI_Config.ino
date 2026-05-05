#include <TFT_eSPI.h> 
#include <DHT.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h> 
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>

TFT_eSPI tft = TFT_eSPI(); 

// ==========================================
// 1. CONFIGURACIÓN DE NUBE (THINGSPEAK)
// ==========================================
String apiKey = "WPJLX77LHQVRNEK1"; 
const char* server = "http://api.thingspeak.com/update";

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
int ultimoPixelX = 0; // Para saber qué botón estaba presionando

// Variables Globales de Nube
float ultimaTemp = -100.0;
float ultimaHum = -100.0;
int ultimoGas = -1;

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
  
  // WiFiManager auto-conecta si ya hay clave guardada
  WiFiManager wm;
  wm.setConnectTimeout(10);
  if(wm.autoConnect("ESP32_Smart_Monitor")) {
      wifiActivado = true;
  }

  // Dibujamos la interfaz EXACTA que teníamos
  dibujarInterfazBasica();
  dibujarGuiaCables();
}

void loop() {
  // --- 1. LECTURA TÁCTIL (Corta vs Larga) ---
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    ultimoPixelX = map(p.x, 300, 3800, 0, 320);

    if (!estaTocando) {
      tiempoInicioToque = millis();
      estaTocando = true;
    }
    
    // Si mantiene presionado más de 5 segundos Y está tocando el lado derecho (Botón WiFi)
    if (millis() - tiempoInicioToque > 5000 && ultimoPixelX > 160) {
      activarModoConfiguracion(); 
      estaTocando = false; 
    }
  } else {
    // Si soltó el dedo ANTES de los 5 segundos (Toque normal)
    if (estaTocando) {
      unsigned long duracion = millis() - tiempoInicioToque;
      
      if (duracion < 5000) { 
        if (ultimoPixelX < 160) {
          // Cambiar sensor
          modoActual = (modoActual == 0) ? 1 : 0;
        } else {
          // Encender/Apagar WiFi
          wifiActivado = !wifiActivado;
          if(wifiActivado) WiFi.begin(); else WiFi.disconnect();
        }
        
        tft.fillScreen(TFT_BLACK); 
        dibujarInterfazBasica();
        dibujarGuiaCables();
      }
      estaTocando = false;
    }
  }

  // --- 2. REFRESCO DE PANTALLA (CADA 2 SEGUNDOS) ---
  if (millis() - ultimoTiempoPantalla > 2000) { 
    ultimoTiempoPantalla = millis();
    tft.fillRect(10, 60, 300, 85, TFT_BLACK); // Borrado parcial anti-flicker

    if (modoActual == 0) {
      mostrarDatosDHT();
    } else {
      mostrarDatosGas();
    }
  }

  // --- 3. TRANSMISIÓN NUBE (CADA 15 SEGUNDOS) ---
  if (millis() - ultimoTiempoNube > 15000) {
    ultimoTiempoNube = millis();
    if (wifiActivado && WiFi.status() == WL_CONNECTED) {
      enviarDatosNube();
    }
  }
}

// ==========================================
// 3. PORTAL CAUTIVO (NUEVA FUNCIÓN, SIN DAÑAR UI)
// ==========================================
void activarModoConfiguracion() {
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("MODO CONFIGURACION", 160, 40, 2);
  tft.setTextSize(1);
  tft.drawCentreString("Conectate con tu celular al WiFi:", 160, 90, 2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString("ESP32_Setup", 160, 120, 4);
  
  WiFiManager wm;
  if (!wm.startConfigPortal("ESP32_Setup")) {
    ESP.restart(); // Si falla, reiniciamos la placa por seguridad
  }
  
  // Si se configuró con éxito, restauramos la UI
  wifiActivado = true;
  tft.fillScreen(TFT_BLACK);
  dibujarInterfazBasica();
  dibujarGuiaCables();
}

void enviarDatosNube() {
  HTTPClient http;
  String url = String(server) + "?api_key=" + apiKey;
  if (modoActual == 0 && ultimaTemp != -100.0) {
    url += "&field1=" + String(ultimaTemp) + "&field2=" + String(ultimaHum);
  } else if (modoActual == 1 && ultimoGas != -1) {
    url += "&field3=" + String(ultimoGas);
  }
  http.begin(url);
  http.GET(); 
  http.end();
}

// ==========================================
// 4. FUNCIONES DE INTERFAZ GRÁFICA (100% ORIGINALES)
// ==========================================
void dibujarInterfazBasica() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2); 
  tft.drawCentreString(modoActual == 0 ? "MONITOR: TEMPERATURA" : "MONITOR: GAS AMBIENTAL", 160, 10, 2);
  tft.drawFastHLine(10, 40, 300, TFT_BLUE); 
  
  // Botones Inferiores intactos
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
  } else {
    tft.drawCentreString("GUIA GAS (Sensor Azul):", 160, 145, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawCentreString("ROJO->VCC | NEGRO->GND | AMARILLO->AO", 160, 165, 2);
  }
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

void mostrarDatosGas() {
  int nivelGas = analogRead(DATA_PIN); 
  tft.setTextSize(1); 
  
  if (nivelGas < 200 || nivelGas >= 4090) { 
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("DESCONECTADO", 160, 80, 4);
      tft.fillRect(20, 115, 280, 20, TFT_BLACK); // Borramos barra anterior
      ultimoGas = -1;
  } else {
      ultimoGas = nivelGas;
      
      // Lógica de colores intacta
      uint16_t colorGas = TFT_GREEN;           
      if (nivelGas > 1500) colorGas = TFT_RED; 
      else if (nivelGas > 950) colorGas = TFT_ORANGE; 
      
      tft.setTextColor(colorGas, TFT_BLACK);
      tft.drawCentreString("Nivel: " + String(nivelGas), 160, 65, 4);

      // Gráfico de Barras intacto
      tft.drawRect(20, 115, 280, 20, TFT_WHITE);
      int anchoBarra = map(nivelGas, 750, 4090, 0, 276); 
      if (anchoBarra < 0) anchoBarra = 0;
      if (anchoBarra > 276) anchoBarra = 276;
      tft.fillRect(22, 117, anchoBarra, 16, colorGas);
      
      Serial.print("DATA,G,"); Serial.println(nivelGas);
  }
}