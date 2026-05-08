<?php
// Ocultamos errores feos de PHP y forzamos salida limpia en JSON
error_reporting(0);
header("Access-Control-Allow-Origin: *");
header("Content-Type: application/json; charset=UTF-8");

try {
    $servername = "mysql.comunidadingenieria.cl";
    $username = "grupo2top3";
    $password = "Unab.2026*";
    $dbname = "grupo2top3";

    // REVISA ESTO EN TU PHPMYADMIN: ¿Se llama así tu tabla?
    $tabla = "lecturas_sensores";

    // Crear conexión
    $conn = new mysqli($servername, $username, $password, $dbname);

    // Verificar conexión a la BD
    if ($conn->connect_error) {
        echo json_encode(array("error" => "Error de credenciales: " . $conn->connect_error));
        exit();
    }

    $sql = "SELECT * FROM " . $tabla . " ORDER BY fecha DESC LIMIT 2000";
    $result = $conn->query($sql);

    // Si la tabla no existe o hay un error SQL, atrapamos el error aquí
    if (!$result) {
        echo json_encode(array("error" => "Error SQL (¿Existe la tabla?): " . $conn->error));
        exit();
    }

    $datos = array();
    while ($row = $result->fetch_assoc()) {
        $row['valor'] = floatval($row['valor']);
        $datos[] = $row;
    }

    $conn->close();
    echo json_encode($datos);

} catch (Exception $e) {
    // Si cualquier otra cosa explota, lo capturamos
    echo json_encode(array("error" => "Error Crítico PHP: " . $e->getMessage()));
}
?>