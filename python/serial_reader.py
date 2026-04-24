import serial
import time
import mysql.connector
from mysql.connector import Error

# ==========================================
# 1. CONFIGURACIÓN DEL SISTEMA
# ==========================================

# Puerto Serial (Asegúrate de que sea el correcto en tu PC)
PUERTO_SERIAL = 'COM3' 
BAUD_RATE = 115200

# Configuración de MySQL (XAMPP)
DB_CONFIG = {
    "host": "localhost",
    "user": "root",
    "password": "",
    "database": "lab_iot_master"
}

# ==========================================
# 2. FUNCIONES DE BASE DE DATOS
# ==========================================

def conectar_bd():
    """Establece conexión con el servidor MySQL de XAMPP."""
    try:
        conexion = mysql.connector.connect(**DB_CONFIG)
        if conexion.is_connected():
            return conexion
    except Error as e:
        print(f"❌ [DB ERROR] No se pudo conectar a MySQL: {e}")
    return None

def insertar_clima(conexion, temp, hum):
    """Inserta datos en la tabla especializada de Clima."""
    if conexion and conexion.is_connected():
        try:
            cursor = conexion.cursor()
            query = "INSERT INTO registro_clima (temperatura, humedad) VALUES (%s, %s)"
            cursor.execute(query, (temp, hum))
            conexion.commit()
            print(f" ✅ [MYSQL] Clima guardado: {temp}°C, {hum}%")
        except Error as e:
            print(f" ⚠️ [MYSQL ERROR] Error en tabla clima: {e}")

def insertar_gas(conexion, gas):
    """Inserta datos en la tabla especializada de Gas."""
    if conexion and conexion.is_connected():
        try:
            cursor = conexion.cursor()
            query = "INSERT INTO registro_gas (nivel_gas) VALUES (%s)"
            cursor.execute(query, (gas,))
            conexion.commit()
            print(f" ✅ [MYSQL] Gas guardado: {gas} unidades")
        except Error as e:
            print(f" ⚠️ [MYSQL ERROR] Error en tabla gas: {e}")

# ==========================================
# 3. BUCLE PRINCIPAL DE EJECUCIÓN
# ==========================================

def iniciar_lector():
    conexion_esp = None
    db_conn = None

    try:
        # Configuración del Puerto Serial
        print(f"🚀 Iniciando conexión en {PUERTO_SERIAL}...")
        conexion_esp = serial.Serial(PUERTO_SERIAL, BAUD_RATE, timeout=1)
        
        # Parche Master IoT: Evita reinicio de la ESP32 al abrir puerto
        conexion_esp.setDTR(False)
        conexion_esp.setRTS(False)
        
        time.sleep(2) # Pausa de estabilización
        print("📡 Puerto Serial abierto correctamente.")

        # Conexión inicial a Base de Datos
        db_conn = conectar_bd()
        if db_conn:
            print("🗄️  Conexión a MySQL (XAMPP) establecida.")
        
        print("\n" + "="*45)
        print("  ESCUCHANDO DATOS DE SENSORES EN VIVO")
        print("="*45 + "\n")

        while True:
            if conexion_esp.in_waiting > 0:
                # Leer y limpiar la línea recibida
                linea = conexion_esp.readline().decode('utf-8').strip()

                # Solo procesamos si empieza con nuestra cabecera DATA
                if linea.startswith("DATA"):
                    partes = linea.split(",")
                    
                    # CASO A: Datos de Temperatura/Humedad
                    if partes[1] == 'T':
                        try:
                            t = float(partes[2])
                            h = float(partes[4])
                            print(f"[DHT22] LECTURA -> Temp: {t} | Hum: {h}", end="")
                            insertar_clima(db_conn, t, h)
                        except (IndexError, ValueError):
                            print(" ⚠️ Error al procesar datos de Clima")

                    # CASO B: Datos de Gas
                    elif partes[1] == 'G':
                        try:
                            g = int(partes[2])
                            print(f"[MQ-GAS] LECTURA -> Nivel: {g}", end="")
                            insertar_gas(db_conn, g)
                        except (IndexError, ValueError):
                            print(" ⚠️ Error al procesar datos de Gas")

    except serial.SerialException as e:
        print(f"\n❌ [ERROR SERIAL] ¿El Monitor de Arduino está abierto?: {e}")
    except KeyboardInterrupt:
        print("\n👋 Programa detenido por el usuario.")
    finally:
        # Cierre seguro de conexiones
        if conexion_esp and conexion_esp.is_open:
            conexion_esp.close()
            print("🔌 Puerto Serial cerrado.")
        if db_conn and db_conn.is_connected():
            db_conn.close()
            print("🗄️  Conexión MySQL cerrada.")

if __name__ == "__main__":
    iniciar_lector()
