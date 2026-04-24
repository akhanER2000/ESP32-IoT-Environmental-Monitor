-- 1. Crear la base de datos si no existe
CREATE DATABASE IF NOT EXISTS lab_iot_master;
USE lab_iot_master;

-- 2. Limpieza: Borramos la tabla antigua "todo en uno" para evitar confusiones
DROP TABLE IF EXISTS registro_sensores;

-- 3. Tabla especializada para el sensor DHT22 (Temperatura y Humedad)
CREATE TABLE IF NOT EXISTS registro_clima (
    id INT AUTO_INCREMENT PRIMARY KEY,
    temperatura FLOAT NOT NULL,
    humedad FLOAT NOT NULL,
    fecha_hora TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 4. Tabla especializada para el sensor MQ (Nivel de Gas Analógico)
CREATE TABLE IF NOT EXISTS registro_gas (
    id INT AUTO_INCREMENT PRIMARY KEY,
    nivel_gas INT NOT NULL,
    fecha_hora TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);