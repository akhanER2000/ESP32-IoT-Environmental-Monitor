# 🌡️ ESP32 Smart Environmental Monitor

Este proyecto consiste en una estación de monitoreo IoT híbrida, que utiliza una placa **ESP32-2432S028** (con pantalla TFT táctil) para medir, visualizar y transmitir datos ambientales hacia un servidor en producción.

## 🚀 Características y Arquitectura
- **Arquitectura de Ingesta (Nube):** Se ha eliminado la dependencia de ThingSpeak y servidores locales XAMPP. Ahora, la ESP32 envía directamente las lecturas mediante peticiones **HTTP POST (HTTPS)** a nuestro servidor remoto universitario (`grupo2top3.comunidadingenieria.cl`).
- **Backend PHP:** Se implementó una API robusta (`ingesta.php`) construida con **PDO** para la inserción segura en MySQL. Además, este backend gestiona automáticamente la zona horaria (`America/Santiago`), insertando la marca de tiempo (timestamp) de forma precisa para asegurar la integridad absoluta de la serie de tiempo.
- **Firmware Avanzado:** 
  - **WiFiManager:** Incluye un Portal Cautivo que permite configurar las credenciales de red desde el celular, sin necesidad de reprogramar la placa.
  - **WiFiClientSecure:** Implementado para soportar conexiones cifradas y evadir restricciones de certificados en las peticiones HTTPS hacia el servidor.
- **Interfaz Táctil Dinámica:** Cambio intuitivo entre el monitor de Clima (DHT22) y el de Gas (MQ), con reportes visuales en tiempo real.

## 🎯 Objetivo Big Data
El sistema completo fue diseñado para operar de forma autónoma e ininterrumpida durante **5 días continuos**. El propósito es generar un dataset masivo, óptimo y sin huecos (gaps) para su posterior análisis, limpieza y modelado con el ecosistema de **Apache Spark / PySpark**.

## 🛠️ Hardware Utilizado
- **Controlador:** ESP32-2432S028 (Cheap Yellow Display).
- **Sensor 1:** DHT22 (Temperatura y Humedad).
- **Sensor 2:** MQ-Series (Gas Analógico).
- **Conexión:** WiFi (2.4 GHz) con soporte de encriptación TLS/SSL.

## 📂 Estructura del Proyecto
- `/firmware`: Código `.ino` para Arduino IDE (incluye lógica de red y conexión segura).
- `/backend`: Scripts PHP (`ingesta.php`) listos para el servidor universitario.
- `/database`: Esquema SQL y consultas de administración.
- `/python`: Scripts legacy y utilidades opcionales.

## 📖 Instrucciones de Uso (Configuración WiFi / Portal Cautivo)
1. **Activar WiFi:** Toca el botón "WIFI" en la pantalla táctil.
2. **Modo Configuración:** Mantén presionado el mismo botón táctil durante **5 segundos**. La pantalla cambiará a un fondo azul indicando el "MODO CONFIGURACION".
3. **Punto de Acceso:** La placa creará automáticamente una red inalámbrica llamada `ESP32_Setup`.
4. **Conexión Móvil:** Usa tu smartphone para conectarte a esta red. Se desplegará un menú automático (o ingresa a `192.168.4.1`) donde podrás introducir la clave del WiFi del lugar donde te encuentres.
5. **Transmisión Autónoma:** Tras conectarse con éxito, la ESP32 guardará la configuración y comenzará el envío ininterrumpido a nuestra API en PHP.
