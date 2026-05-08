# 🌡️ ESP32 Smart Environmental Monitor

> ⚠️ **NOTA DE EJECUCIÓN:** El notebook principal de este proyecto (`Grupo2_Evaluacion_Sumativa.ipynb`) ha sido diseñado exclusivamente para **Google Colab**. Debido a las dependencias de red, infraestructura de Java y el uso de endpoints HTTPS para eludir firewalls, **NO debe ejecutarse en entornos locales**. Para pruebas en PC local, utilice únicamente la versión adaptada `Grupo2_Sumativa_LOCAL.ipynb` bajo su propio riesgo de configuración.

Este proyecto consiste en una estación de monitoreo IoT híbrida, que utiliza una arquitectura de **Red de Nodos (Maestro-Esclavo)** para medir, visualizar y transmitir datos ambientales hacia un servidor en producción.

## 🚀 Arquitectura y Topología de Red
- **Gateway Maestro (CYD):** La placa ESP32 principal (con pantalla táctil) actúa como un Gateway inteligente. Recolecta sus propios datos locales y escucha peticiones inalámbricas de otros nodos mediante el protocolo de latencia ultrabaja **ESP-NOW**.
- **Nodo Esclavo (Remoto):** Una segunda placa ESP32 en protoboard, encargada de recolectar datos remotos (Gas y Humedad de Tierra), que transmite al Maestro de forma instantánea y sin depender de una red WiFi local.
- **Ingesta en Segundo Plano (Background Sync):** El Maestro procesa y envía un *payload* completo a nuestra API remota cada 15 segundos sin interrumpir la interfaz de usuario (UI), recolectando datos de todos los sensores sin importar en qué pantalla se encuentre el usuario.
- **Backend PHP (REST API):** API robusta (`ingesta.php` / `api_sensores.php`) construida con **PDO** para inserción segura en MySQL remoto, con gestión automática de zona horaria (`America/Santiago`).

## 📡 Estandarización de Datos
Los identificadores enviados vía POST al servidor PHP (`grupo2top3.comunidadingenieria.cl`) han sido estrictamente estandarizados para asegurar la coherencia en la Base de Datos:
- `Local_DHT22_Temp`: Temperatura del aire local (DHT22 conectado al Maestro).
- `Local_DHT22_Hum`: Humedad del aire local (DHT22 conectado al Maestro).
- `Local_MQ135`: Nivel de gas detectado por el sensor analógico del Maestro.
- `Remoto_MQ135`: Nivel de gas detectado por el Nodo Esclavo.
- `Remoto_Humedad_Tierra`: Nivel de humedad del suelo detectado por el Nodo Esclavo.

## 🛠️ Hardware Utilizado
- **Gateway (Maestro):** ESP32-2432S028 (Cheap Yellow Display).
- **Esclavo:** Placa ESP32 estándar (Protoboard).
- **Sensores Locales:** DHT22 y MQ135.
- **Sensores Remotos:** MQ135 y Sensor de Humedad de Tierra.
- **Conectividad:** ESP-NOW (comunicación entre placas) + WiFi/HTTPS (conexión nube).

## 📂 Estructura del Proyecto
- `/firmware/master_cyd`: Código `.ino` del Gateway (ESP32 con Pantalla táctil).
- `/firmware/slave_sensor`: Código `.ino` del Nodo Remoto.
- `/backend`: Scripts PHP (`ingesta.php`, `api_sensores.php`) alojados en el servidor universitario.
- `/database`: Esquema SQL unificado (`lecturas_sensores`) para el backend.
- `/Notebook`: Notebooks de Apache Spark para la evaluación sumativa (Colab y Local).
- `/python`: Scripts legacy de captura serial y generación de reportes.

## 🔧 Dependencias
### Notebook (PySpark / Evaluación Sumativa)
```
pyspark, pandas, requests, findspark, pymongo
```
### Scripts Python Locales
```
pyserial, mysql-connector-python, matplotlib
```
Instalar con: `pip install -r python/requirements.txt`

## 📖 Instrucciones de Uso (Configuración WiFi / Portal Cautivo)
1. **Activar WiFi:** Toca el botón "WIFI" en la pantalla táctil del Maestro.
2. **Modo Configuración:** Mantén presionado el mismo botón táctil durante **5 segundos**. La pantalla cambiará a un fondo azul indicando el "MODO CONFIGURACION".
3. **Punto de Acceso:** La placa creará automáticamente una red inalámbrica llamada `ESP32_Setup`.
4. **Conexión Móvil:** Usa tu smartphone para conectarte a esta red y navega a `192.168.4.1` (si no se abre automáticamente) para introducir la clave de tu router local.
5. **Transmisión Autónoma:** Tras conectarse con éxito, el Maestro guardará la configuración y comenzará el envío ininterrumpido en segundo plano.

## 💡 Mejoras Recientes de UI y Hardware
- **Resolución ADC2:** Implementado un *workaround* de lectura en el sensor de gas local que soluciona de forma definitiva los conflictos nativos cuando la antena WiFi está activada.
- **Unificación Visual:** Las pantallas del sensor de gas (tanto local como remoto) ahora comparten un elegante diseño de "barras de señal" dinámicas (verde/amarillo/rojo) para una rápida interpretación de la peligrosidad del aire.
- **Channel Hopping:** El nodo esclavo implementa un algoritmo de auto-recuperación que escanea los canales WiFi (1-13) para reencontrar al Maestro tras desconexiones.
