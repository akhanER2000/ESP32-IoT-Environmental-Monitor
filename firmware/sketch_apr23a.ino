#include <TFT_eSPI.h> 
#include <DHT.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h> 

TFT_eSPI tft = TFT_eSPI(); 

// --- Configuración de Sensores ---
#define DATA_PIN 27    // Pin IO27 del puerto CN1
#define DHTTYPE DHT22
DHT dht(DATA_PIN, DHTTYPE);

// --- Pines Táctiles Fijos ---
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

int modoActual = 0; 
unsigned long ultimoTiempo = 0; 

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
  
  dibujarInterfazBasica();
  dibujarGuiaCables();
}

void loop() {
  if (ts.tirqTouched() && ts.touched()) {
    modoActual = (modoActual == 0) ? 1 : 0;
    tft.fillScreen(TFT_BLACK); 
    dibujarInterfazBasica();
    dibujarGuiaCables();
    delay(400); 
  }

  if (millis() - ultimoTiempo > 2000) {
    ultimoTiempo = millis();
    
    // UI FIX: Aumentamos la altura de borrado a 85 píxeles para limpiar bien la barra y el texto
    tft.fillRect(10, 60, 300, 85, TFT_BLACK); 

    if (modoActual == 0) {
      mostrarDatosDHT();
    } else {
      mostrarDatosGas();
    }
  }
}

void dibujarInterfazBasica() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2); 
  tft.drawCentreString(modoActual == 0 ? "MONITOR: TEMPERATURA" : "MONITOR: GAS AMBIENTAL", 160, 10, 2);
  tft.drawFastHLine(10, 40, 300, TFT_BLUE); 
  
  tft.fillRoundRect(50, 190, 220, 40, 5, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawCentreString("TOCA PARA CAMBIAR", 160, 200, 1);
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
  
  tft.setTextSize(1); // Reseteamos multiplicador para evitar letras gigantes

  if (isnan(h) || isnan(t)) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString("DESCONECTADO", 160, 80, 4);
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(30, 65);  tft.print("Temp: " + String(t, 1) + " C");
    tft.setCursor(30, 100); tft.print("Hum:  " + String(h, 1) + " %");
    
    Serial.print("DATA,T,"); Serial.print(t); Serial.print(",H,"); Serial.println(h);
  }
}

void mostrarDatosGas() {
  int nivelGas = analogRead(DATA_PIN); 
  
  tft.setTextSize(1); // UI FIX: Tamaño normalizado para evitar superposición
  
  if (nivelGas < 200) { 
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("DESCONECTADO", 160, 80, 4);
  } else {
      // 1. Lógica de Colores Dinámicos (Calibrada a tu entorno)
      uint16_t colorGas = TFT_GREEN;           // 798 - 950 (Normal)
      if (nivelGas > 1500) colorGas = TFT_RED; // Peligro (Cerca de alcohol/gas)
      else if (nivelGas > 950) colorGas = TFT_ORANGE; // Precaución
      
      // 2. Imprimir texto ajustado en altura (Y=65)
      tft.setTextColor(colorGas, TFT_BLACK);
      tft.drawCentreString("Nivel: " + String(nivelGas), 160, 65, 4);

      // 3. Dibujar Gráfico de Barras más abajo (Y=115)
      tft.drawRect(20, 115, 280, 20, TFT_WHITE);
      
      // Mapeo matemático ajustado a tu Línea Base (~800)
      int anchoBarra = map(nivelGas, 750, 4095, 0, 276);
      
      // Evitar bugs visuales limitando el ancho entre 0 y 276 px
      if (anchoBarra < 0) anchoBarra = 0;
      if (anchoBarra > 276) anchoBarra = 276;
      
      // Rellenar el gráfico
      tft.fillRect(22, 117, anchoBarra, 16, colorGas);
      
      Serial.print("DATA,G,"); Serial.println(nivelGas);
  }
}