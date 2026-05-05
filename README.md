# 🌡️ ESP32 Smart Environmental Monitor

Este proyecto consiste en una estación de monitoreo IoT híbrida ("Full-Stack" y Nube) que utiliza una placa **ESP32-2432S028** (con pantalla TFT táctil) para medir, visualizar y almacenar datos ambientales.

## 🚀 Características
- **Interfaz Táctil Dinámica:** Cambio entre monitor de Clima (DHT22) y Gas (MQ).
- **Dashboard en Tiempo Real:** Gráficos de barras y alertas de color según la peligrosidad.
- **Transmisión Híbrida/Paralela:** 
  - **Local:** Envío de datos por cable Serial a un puente en Python para registro en **MySQL**.
  - **Nube:** Envío de lecturas por WiFi hacia **ThingSpeak** para monitoreo global.
- **Cronómetros Asíncronos:** Refresco en pantalla cada 2 segundos y transmisión a la nube cada 15 segundos, sin bloquear la interfaz.
- **Portal Cautivo (WiFiManager):** Configuración de credenciales de WiFi directamente desde tu smartphone, sin necesidad de reprogramar el código.

## 🛠️ Hardware Utilizado
- **Controlador:** ESP32-2432S028 (Cheap Yellow Display).
- **Sensor 1:** DHT22 (Temperatura y Humedad).
- **Sensor 2:** MQ-Series (Gas Analógico).
- **Conexión:** USB-Serial y WiFi (2.4 GHz).

## 📂 Estructura del Proyecto
- `/firmware`: Código `.ino` para Arduino IDE.
- `/python`: Scripts para la captura y reporte de datos locales.
- `/database`: Esquema SQL para la base de datos.
- `/documentos`: Reportes generados (CSV e imágenes).

## 🔧 Dependencias y Requisitos
### Arduino IDE
Asegúrate de instalar las siguientes librerías desde el Gestor de Librerías:
- `TFT_eSPI`
- `DHT sensor library`
- `XPT2046_Touchscreen`
- **`WiFiManager`** (de tzapu) - *¡NUEVO!*

### Servicios en la Nube
- **ThingSpeak:** Es necesario crear una cuenta gratuita en [ThingSpeak](https://thingspeak.com/) y configurar los "Fields" de tu canal para recibir los datos de temperatura, humedad y gas.

### Entorno Python (Base de Datos Local)
```bash
python -m venv iot_env
# En Windows: 
.\iot_env\Scripts\activate
# Instalar dependencias:
pip install -r python/requirements.txt
```

## 📖 Instrucciones de Uso (Configuración WiFi)

Con la nueva actualización, el dispositivo es capaz de conectarse a internet de forma autónoma sin necesidad de reescribir el código con las contraseñas de tu casa:

1. **Encender el WiFi:** Toca el botón de WIFI en la parte inferior derecha de la pantalla para activar el módulo inalámbrico (verás que cambia a "WIFI: ON").
2. **Entrar al Modo Configuración:** Mantén presionado el mismo botón táctil de WIFI durante **5 segundos continuos**.
3. **Portal Cautivo:** La pantalla cambiará a un fondo azul indicando el "MODO CONFIGURACION". En este punto, el ESP32 crea su propia red inalámbrica.
4. **Conexión desde el Celular:** 
   - Busca en tu teléfono móvil las redes WiFi disponibles y conéctate a la red llamada **`ESP32_Setup`**.
   - Automáticamente se abrirá una ventana en tu navegador (si no lo hace, ingresa a `192.168.4.1`).
   - Selecciona "Configure WiFi", elige la red de tu casa e ingresa tu contraseña.
5. **Listo:** El dispositivo guardará las credenciales internamente, restablecerá su interfaz y comenzará a enviar datos a ThingSpeak automáticamente.

## 🗄️ Instalación Local (MySQL)
1. Carga el firmware en la ESP32 usando Arduino IDE.
2. Importa el esquema SQL en tu servidor MySQL (XAMPP).
3. Ejecuta los scripts de Python para iniciar la captura Serial.
