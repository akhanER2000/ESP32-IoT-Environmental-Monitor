-- 1. Crear la base de datos (Opcional si ya usas la del servidor)
CREATE DATABASE IF NOT EXISTS lab_iot_master;
USE lab_iot_master;

-- 2. Limpieza: Borramos tablas antiguas para evitar confusiones
DROP TABLE IF EXISTS registro_clima;
DROP TABLE IF EXISTS registro_gas;
DROP TABLE IF EXISTS lecturas_sensores;

-- 3. Tabla unificada para inyección desde API (PHP PDO)
CREATE TABLE IF NOT EXISTS lecturas_sensores (
    id INT AUTO_INCREMENT PRIMARY KEY,
    dispositivo VARCHAR(50) NOT NULL,
    valor FLOAT NOT NULL,
    fecha DATETIME NOT NULL
);

-- 4. Registros de ejemplo (Datos simulados para validación con los nuevos identificadores)
INSERT INTO lecturas_sensores (dispositivo, valor, fecha) VALUES
('Local_DHT22_Temp', 24.5, NOW()),
('Local_DHT22_Hum', 60.2, NOW()),
('Local_MQ135', 450.0, NOW()),
('Remoto_MQ135', 380.0, NOW()),
('Remoto_Humedad_Tierra', 75.0, NOW());