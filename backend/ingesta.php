<?php
// 1. FORZAMOS LA HORA DE CHILE EN PHP
date_default_timezone_set('America/Santiago');

// Credenciales del Grupo 2
$host = 'mysql.comunidadingenieria.cl';
$db = 'grupo2top3';
$user = 'grupo2top3';
$pass = 'Unab.2026*';
$charset = 'utf8mb4';

$dsn = "mysql:host=$host;dbname=$db;charset=$charset";
$options = [
    PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
    PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
];

try {
    $pdo = new PDO($dsn, $user, $pass, $options);
} catch (\PDOException $e) {
    echo json_encode(["status" => "error", "message" => $e->getMessage()]);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $dispositivo = isset($_POST['dispositivo']) ? $_POST['dispositivo'] : 'Desconocido';
    $valor = isset($_POST['valor']) ? (float) $_POST['valor'] : 0.0;

    // 2. GENERAMOS LA HORA ACTUAL DESDE PHP (YA EN HORARIO DE CHILE)
    $fecha_actual = date("Y-m-d H:i:s");

    // 3. INSERTAMOS LA FECHA MANUALMENTE (Esto sobreescribe el reloj del servidor)
    $stmt = $pdo->prepare("INSERT INTO lecturas_sensores (dispositivo, valor, fecha) VALUES (?, ?, ?)");
    $stmt->execute([$dispositivo, $valor, $fecha_actual]);

    echo json_encode(["status" => "success", "time_applied" => $fecha_actual]);
} else {
    echo json_encode(["status" => "error", "message" => "Solo se acepta POST"]);
}
?>