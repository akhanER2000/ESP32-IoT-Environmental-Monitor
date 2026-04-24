import mysql.connector
import pandas as pd
import matplotlib.pyplot as plt
import os

# ==========================================
# 1. CONFIGURACIÓN Y RUTAS (CORREGIDAS)
# ==========================================
DB_CONFIG = {
    "host": "localhost",
    "user": "root",
    "password": "",
    "database": "lab_iot_master"
}

# --- LA MAGIA DE LAS RUTAS ABSOLUTAS ---
# 1. Obtenemos la ruta exacta de donde está este archivo (la carpeta 'python/')
DIRECTORIO_ACTUAL = os.path.dirname(os.path.abspath(__file__))

# 2. Subimos un nivel para llegar a la raíz del proyecto (la carpeta principal)
DIRECTORIO_RAIZ = os.path.dirname(DIRECTORIO_ACTUAL)

# 3. Construimos la ruta exacta hacia la carpeta 'documentos' en la raíz
CARPETA_DESTINO = os.path.join(DIRECTORIO_RAIZ, "documentos")

def generar_reportes():
    try:
        # Crear la carpeta en la raíz si no existe
        if not os.path.exists(CARPETA_DESTINO):
            os.makedirs(CARPETA_DESTINO)
            print(f"📁 Carpeta raíz creada en: {CARPETA_DESTINO}")

        print("🗄️ Conectando a MySQL para extraer datos...")
        conexion = mysql.connector.connect(**DB_CONFIG)
        cursor = conexion.cursor(dictionary=True)

        # ==========================================
        # 2. EXTRACCIÓN DE DATOS
        # ==========================================
        cursor.execute("SELECT fecha_hora, temperatura, humedad FROM registro_clima ORDER BY fecha_hora ASC")
        df_clima = pd.DataFrame(cursor.fetchall())
        
        cursor.execute("SELECT fecha_hora, nivel_gas FROM registro_gas ORDER BY fecha_hora ASC")
        df_gas = pd.DataFrame(cursor.fetchall())

        if df_clima.empty and df_gas.empty:
            print("⚠️ La base de datos está vacía.")
            return

        # ==========================================
        # 3. EXPORTACIÓN A CSV
        # ==========================================
        if not df_clima.empty:
            ruta_clima = os.path.join(CARPETA_DESTINO, "reporte_clima.csv")
            df_clima.to_csv(ruta_clima, index=False)
            print(f" ✅ Archivo guardado: '{ruta_clima}'")
        
        if not df_gas.empty:
            ruta_gas = os.path.join(CARPETA_DESTINO, "reporte_gas.csv")
            df_gas.to_csv(ruta_gas, index=False)
            print(f" ✅ Archivo guardado: '{ruta_gas}'")

        # ==========================================
        # 4. GENERACIÓN DE LA GRÁFICA
        # ==========================================
        print("📊 Generando gráfica profesional...")
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
        fig.suptitle('Dashboard de Análisis IoT - Estación Ambiental', fontsize=16, fontweight='bold')

        if not df_clima.empty:
            ax1.plot(df_clima['fecha_hora'], df_clima['temperatura'], label='Temp (°C)', color='#e74c3c', linewidth=2)
            ax1.plot(df_clima['fecha_hora'], df_clima['humedad'], label='Hum (%)', color='#3498db', linewidth=2)
            ax1.set_title('Historial de Clima (DHT22)')
            ax1.legend()
            ax1.grid(True, alpha=0.3)

        if not df_gas.empty:
            ax2.plot(df_gas['fecha_hora'], df_gas['nivel_gas'], label='Nivel Gas', color='#f39c12', linewidth=2)
            ax2.axhline(y=950, color='orange', linestyle='--', label='Precaución')
            ax2.axhline(y=1500, color='red', linestyle='--', label='Peligro')
            ax2.set_title('Monitoreo de Gas (MQ)')
            ax2.legend()
            ax2.grid(True, alpha=0.3)

        plt.tight_layout(rect=[0, 0.03, 1, 0.95])
        
        # Guardar imagen en la carpeta documentos (en la raíz)
        ruta_grafica = os.path.join(CARPETA_DESTINO, "grafica_laboratorio.png")
        plt.savefig(ruta_grafica, dpi=300)
        print(f" 📈 Gráfica guardada en '{ruta_grafica}'")
        
        # plt.show() # Comentado para que termine el script automáticamente sin detenerse, puedes descomentarlo si quieres que abra la ventana.

    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        if 'conexion' in locals() and conexion.is_connected():
            conexion.close()

if __name__ == "__main__":
    generar_reportes()