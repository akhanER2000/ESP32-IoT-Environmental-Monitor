# 🌡️ ESP32 Smart Environmental Monitor

Este proyecto consiste en una estación de monitoreo IoT "Full-Stack" que utiliza una placa **ESP32-2432S028** (con pantalla TFT táctil) para medir, visualizar y almacenar datos ambientales en una base de datos local.

## 🚀 Características
- **Interfaz Táctil:** Cambio dinámico entre monitor de Clima (DHT22) y Gas (MQ).
- **Dashboard en Tiempo Real:** Gráficos de barras y alertas de color según la peligrosidad.
- **Persistencia de Datos:** Puente en Python que envía lecturas a **MySQL (XAMPP)**.
- **Análisis Automatizado:** Generación de reportes en CSV y gráficas estadísticas en formato PNG.

## 🛠️ Hardware Utilizado
- **Controlador:** ESP32-2432S028 (Cheap Yellow Display).
- **Sensor 1:** DHT22 (Temperatura y Humedad).
- **Sensor 2:** MQ-Series (Gas Analógico).
- **Conexión:** USB-Serial.

## 📂 Estructura del Proyecto
- `/firmware`: Código `.ino` para Arduino IDE.
- `/python`: Scripts para la captura y reporte de datos.
- `/database`: Esquema SQL para la base de datos.
- `/documentos`: Reportes generados (CSV e imágenes).

## 🔧 Instalación
1. Carga el firmware en la ESP32 usando Arduino IDE.
2. Importa el esquema SQL en tu servidor MySQL (XAMPP).
3. Configura el entorno virtual de Python:
   ```bash
   python -m venv iot_env
   # En Windows: 
   .\iot_env\Scripts\activate
   # Instalar dependencias:
   pip install -r python/requirements.txt
   ```
