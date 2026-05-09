# 🔍 Auditoría Técnica — `Grupo2_Evaluacion_Sumativa_Colab.ipynb`

**Auditor:** Senior Data Engineer
**Fecha:** 2026-05-08
**Documento de referencia:** `Evaluación Sumativa.docx` (rúbrica TOP 3 — Big Data, ponderación 20% nota final)
**Notebook auditado:** `Notebook/Grupo2_Evaluacion_Sumativa_Colab.ipynb` (46 celdas, post-refactor de auditoría previa)
**Veredicto global:** ✅ **APTO PARA ENTREGA — NIVEL EXCELENTE PROYECTADO** — los 4 bugs funcionales reportados en la auditoría anterior (C2 NULL, C8 cardinalidad, C9 nodo Local ausente, C10 join roto) están **resueltos**. Quedan 5 mejoras menores no bloqueantes detalladas en §3.

---

## 1. Checklist de Cumplimiento (mapeo directo a la rúbrica del docx)

### 1.1 Fase 0 — Setup y dependencias

| Requisito rúbrica | Implementación | Estado |
|---|---|---|
| Instalación de `pyspark`, `pymongo`, conectores Spark MySQL/MongoDB | `!pip install -q pyspark pymongo` (celda 2) | ✅ OK |
| Inicialización SparkSession | `SparkSession.builder.appName("EvaluacionSumativa_Grupo2").getOrCreate()` (celda 3) | ✅ OK |
| Variables de entorno Java/Spark | Implícitas en Colab (no requiere setup manual) | ✅ OK |

> **Nota:** `spark.jars.packages = mysql:mysql-connector-java:8.0.28` fue **eliminado** correctamente — la ingesta MySQL se hace por API REST y el driver JDBC era código muerto.

### 1.2 Fase 1 — Ingesta de las 4 Fuentes (ID3.1) — **20 registros cada una**

| Fuente rúbrica | Variables requeridas (docx) | Variables implementadas | Conteo | Estado |
|---|---|---|---|---|
| **OpenMeteo (API)** | Temperatura 2m, Humedad relativa 2m, Precipitación, Velocidad viento 10m, Dirección viento 10m, Índice UV | `temperatura_2m`, `humedad_relativa_2m`, `precipitacion`, `viento_10m`, `direccion_viento_10m`, `indice_uv` | 20 ✓ (slicing `[:20]`) | ✅ OK |
| **AQICN (API)** | PM2.5, PM10, O3, NO2, SO2 | `pm25`, `pm10`, `o3`, `no2`, `so2` | 20 (jitter sintético sobre lectura base de Santiago) | ✅ OK con disclaimer |
| **MySQL ESP32 (vía API REST)** | `dispositivo`, `valor`, `fecha` (esquema key-value real) | `id`, `dispositivo`, `valor`, `fecha`, `fuente_origen` | 20 ✓ (`pdf_iot.head(20)`) | ✅ OK — slicing aplicado |
| **MongoDB BPM** | `df_mongo_bpm` con `alturaft` | `usuario`, `bpm`, `alturaft`, `fuente_origen` | 20 ✓ | ✅ OK |

**Avances vs auditoría anterior:**
- ✅ Variable muerta `df_dht_limpio` eliminada → ahora `df_mysql_iot` se construye desde `pdf_iot.head(20)` y se registra como vista.
- ✅ Filtro de dispositivos **tolerante** a ambas convenciones (`Local_DHT22_Temp` ↔ `DHT22_Temp`, `MQ135` ↔ `Local_MQ135`).
- ✅ Casting único en ingest: `pdf_db["valor"] = pd.to_numeric(...).astype(float)` — elimina los `CAST(valor AS DOUBLE)` repetidos en queries downstream.
- ✅ Columna **`fuente_origen` (`real` / `mock`)** añadida en `df_mysql_iot` y `df_mongo_bpm` — auditoría de fallbacks transparente.
- ✅ Mock de respaldo automático cuando la API REST devuelve 0 lecturas IoT (resiliencia).

### 1.3 Fase 2 — Preprocesamiento y Transformación (RA4, ID4.2)

| Requisito rúbrica | Implementación | Estado |
|---|---|---|
| `altura_metros = alturaft × 0.3048` en `df_mongo_bpm` | `withColumn("altura_metros", spark_round(col("alturaft") * 0.3048, 2))` (celda 15) | ✅ OK |
| `condicion_clima` con 4 ramas (CALOR Y HUMEDAD / CALOR SECO / CLIMA MODERADO / CONDICION DESCONOCIDA) | `when().when().when().otherwise()` con la lógica exacta del docx (celda 17) | ✅ OK |

### 1.4 Fase 3 — Vistas Temporales

| Requisito rúbrica | Implementación | Estado |
|---|---|---|
| `createOrReplaceTempView` en cada uno de los 4 DataFrames | `tabla_openmeteo`, `tabla_aqicn`, `tabla_mysql_dht`, `tabla_mongo_bpm` (celda 19) | ✅ OK |
| Vista del unificado | `tabla_unificada` (celda 22) | ✅ OK |

### 1.5 Fase 4 — Integración (RA3) — `df_unificado`

| Requisito rúbrica | Implementación | Estado |
|---|---|---|
| Unificación en un único DataFrame con variables relevantes de las 4 fuentes | `FULL OUTER JOIN` por `id_join` sintético (`monotonically_increasing_id()` + `coalesce(1)`) (celda 22) | ✅ OK |
| **Justificación de la estrategia de integración** | Bloque markdown profesional (celda 21) con tabla comparativa de heterogeneidad y 4 ventajas listadas | ✅ Excelente |
| **Resolución de colisión de columnas** | `df_openmeteo.fecha → fecha_om`, `df_mysql_iot.fecha → fecha_iot` antes del join | ✅ OK — bug latente eliminado |

### 1.6 Fase 5 — Consultas (ID3.1, ID4.1)

> Nota: la rúbrica del docx detalla **C1, C2, C3, C4, C5, C7** explícitamente. C6 y C8 no aparecen literalmente en el docx (posible truncamiento del documento original). El notebook implementa C1–C8 + valor agregado C9–C10, lo que **excede el mínimo exigido**.

| ID | Enunciado rúbrica | Resolución actual | Estado |
|---|---|---|---|
| **C1** | Top 10 registros de `df_openmeteo` ordenados por temperatura DESC | `SELECT * FROM tabla_openmeteo ORDER BY temperatura_2m DESC LIMIT 10` | ✅ OK |
| **C2** | Promedio de humedad para un mes/día específico (libre elección) | Filtro **alineado al rango real** del slicing: `MONTH=5 AND DAY=1` (1 de mayo) | ✅ **BUG RESUELTO** |
| **C3** | Cantidad de registros con precipitación > 2 mm en la última semana | `WHERE precipitacion > 2 AND fecha >= now() - INTERVAL 7 DAYS` | ✅ OK |
| **C4** | Promedio de PM2.5, PM10, O3, NO2, SO2 | 5 promedios sobre `tabla_aqicn` | ✅ OK |
| **C5** | Cantidad de registros con NO2 > 4 | `COUNT(*) WHERE no2 > 4` | ✅ OK |
| **C6** *(extra)* | `df_mongo_bpm` — taquicardia + altura | Sin `GROUP BY usuario` redundante; mock recalibrado a `bpm ∈ [70, 130]` | ✅ **MEJORA APLICADA** |
| **C7** | Primeros 10 registros de `df_openmeteo` con `condicion_clima` | `SELECT fecha, temperatura_2m, humedad_relativa_2m, condicion_clima ... LIMIT 10` | ✅ OK |
| **C8** *(extra)* | Estadísticos DHT22 sobre `tabla_mysql_dht` | `MIN/MAX/AVG/COUNT` sin CAST (cast hecho en ingest); cardinalidad ≤ 20 (vista limitada) | ✅ **BUG RESUELTO** |

### 1.7 Fase 6 — Valor Agregado IoT (C9, C10)

| ID | Objetivo | Resolución actual | Estado |
|---|---|---|---|
| **C9** | Estadísticos del MQ135 Local vs Remoto | `WHERE dispositivo LIKE '%MQ135%' GROUP BY dispositivo` — captura ambos nodos aunque la BD use prefijo `Local_` o no | ✅ **BUG RESUELTO** |
| **C10** | Cruce PM2.5 (AQICN) vs gas interior (MQ135) | Reusa `tabla_unificada` con `fecha_om`/`fecha_iot` aliasados — sin colisión ni vistas auxiliares espurias | ✅ **BUG RESUELTO** |

---

## 2. Análisis de Calidad de Datos

### 2.1 Tipado y casting

| DataFrame | Cast en ingest | Comentario |
|---|---|---|
| `df_openmeteo` | ✅ `astype(float)` por columna (6 numéricas) | Buena práctica defensiva — evita `PySparkTypeError` |
| `df_aqicn` | ✅ `astype(float)` para 5 contaminantes | OK |
| `df_mongo_bpm` | ✅ `str` / `float` / `float` | OK |
| `df_mysql_iot` | ✅ `pd.to_numeric(...).astype(float)` en `valor` | **Mejora aplicada** — ya no se repite `CAST(valor AS DOUBLE)` en C8/C9/C10 |

### 2.2 Manejo de fallbacks sintéticos

| Fuente | Fallback | Trazabilidad |
|---|---|---|
| OpenMeteo | `try/except` → mock de 20 horas con valores sintéticos | ✅ Se imprime `[FALLBACK OPENMETEO]` |
| AQICN | `try/except` interno en `fetch_aqicn()` → valores base por defecto | ⚠️ El fallback no marca `fuente_origen` explícito (mejorable) |
| MongoDB | `try/except` → mock con `bpm ∈ [70, 130]` (cubre taquicardia) | ✅ Marca `fuente_origen = "mock"` |
| MySQL/IoT | Mock de 20 lecturas si la API REST devuelve 0 dispositivos válidos | ✅ Marca `fuente_origen = "mock"` |

### 2.3 Manejo del sensor de tierra (`Remoto_Humedad_Tierra`)

| Aspecto | Estado |
|---|---|
| Incluido en filtro de `disp_validos` | ✅ Sí |
| Llega a `df_mysql_iot` (limit 20) | ⚠️ Depende de la mezcla real de la BD; puede quedar fuera del top 20 si hay desbalance |
| Aparece en queries downstream | ❌ No es referenciado por C1–C10 — el sensor de tierra sigue siendo **dato disponible pero inerte analíticamente** |

**Recomendación opcional:** añadir una **C11** (valor agregado adicional) que muestre estadísticos de humedad de tierra para que el sensor no quede subutilizado:

```sql
SELECT AVG(valor) AS humedad_tierra_avg, MIN(valor), MAX(valor)
FROM tabla_mysql_dht
WHERE dispositivo = 'Remoto_Humedad_Tierra'
```

### 2.4 Coherencia del esquema unificado

| Problema previo | Estado actual |
|---|---|
| Colisión de columnas `fecha` (OpenMeteo string ISO vs MySQL DB timestamp) | ✅ **Resuelto** — `fecha_om` y `fecha_iot` aliasadas antes del join |
| `coalesce(1)` anula paralelismo | Aceptado y documentado en la justificación (válido para 20 filas) |
| Lecturas IoT clavadas (min == max == avg) | Mitigado por casting numérico correcto y `fuente_origen` que delata si son mock |

---

## 3. Puntos Críticos de Mejora (no bloqueantes)

### 3.1 ⚠️ [MEJORA] Consistencia AQICN — falta `fuente_origen`

`df_aqicn` no incluye la columna `fuente_origen`, mientras que `df_mysql_iot` y `df_mongo_bpm` sí. Esto rompe la simetría auditiva.

**Fix sugerido (1 línea):**
```python
pdf_aq["fuente_origen"] = "real" if AQICN_TOKEN != "<PLACEHOLDER_AQICN_TOKEN>" else "mock"
```

### 3.2 ⚠️ [MEJORA] Imports duplicados

`from pyspark.sql.functions import col` se importa en celdas 6, 15, 17, 22 y 44. `monotonically_increasing_id` se importa en 22 y 44. Centralizar en Fase 0 reduce ruido y previene errores al re-ejecutar celdas individuales.

### 3.3 ⚠️ [MEJORA] Naming inconsistente: `df_mysql_dht` vs `df_mysql_iot`

La celda 6 crea `df_mysql_iot` y luego asigna alias `df_mysql_dht = df_mysql_iot`. La vista temporal sigue llamándose `tabla_mysql_dht`. Aunque funciona, el nombre **engaña**: el contenido incluye DHT22 + MQ135 + tierra, no solo DHT.

**Recomendación:** renombrar la vista a `tabla_mysql_iot` y eliminar el alias retro-compatible (forzar la actualización de cualquier consumidor downstream — solo C8/C9/C10 que ya tocamos).

### 3.4 ⚠️ [MEJORA] Pivot opcional para alinearse al texto de la rúbrica

La rúbrica menciona `temperatura_interior` y `humedad` como variables de MySQL. El esquema real es key-value (`dispositivo`, `valor`). Si el evaluador es **estricto literal**, conviene exponer una vista pivotada adicional:

```python
from pyspark.sql.functions import first
df_mysql_pivot = (df_mysql_iot.groupBy("fecha")
    .pivot("dispositivo", ["Local_DHT22_Temp", "Local_DHT22_Hum"])
    .agg(first("valor"))
    .withColumnRenamed("Local_DHT22_Temp", "temperatura_interior")
    .withColumnRenamed("Local_DHT22_Hum",  "humedad"))
df_mysql_pivot.createOrReplaceTempView("tabla_mysql_dht_pivot")
```

Esta vista no rompe lo existente; solo añade una alternativa con los nombres exigidos.

### 3.5 ⚠️ [MEJORA] Documentar `coalesce(1)` como decisión consciente

El comentario en celda 22 explica el `monotonically_increasing_id()` pero no menciona explícitamente que `coalesce(1)` es una **decisión deliberada para datasets pequeños** (no escalable a producción). Añadir un comentario evita que un revisor lo penalice como anti-patrón.

```python
# coalesce(1): forzamos partición única para garantizar IDs contiguos 0..19.
# Aceptable porque cada fuente tiene 20 filas; en producción usaríamos row_number()
# OVER (ORDER BY <clave_temporal>) sobre datasets particionados.
```

---

## 4. Validación de Infraestructura

### 4.1 Bypass del firewall vía API REST PHP (HTTPS)

| Aspecto | Estado |
|---|---|
| Código real usa `https://grupo2top3.comunidadingenieria.cl/api_sensores.php` | ✅ Verificado en celda 6 |
| Endpoint sirve sobre **HTTPS** (puerto 443, no 3306) → evita bloqueo del firewall universitario | ✅ Correcto |
| Backend PHP delega lectura de MySQL → notebook nunca abre conexión JDBC al puerto bloqueado | ✅ Arquitectura sana |
| `spark.jars.packages` (driver JDBC Maven) | ✅ **Eliminado** — código muerto purgado |
| Manejo de error HTTP de la API | ✅ `raise RuntimeError(f"API REST inaccesible: HTTP {status_code}")` — falla rápido y claro |
| Timeout explícito en `requests.get` | ✅ `timeout=30` |

### 4.2 Coherencia README ↔ código ↔ commits

El README del proyecto (`../ESP32_Proyect_pyserial/README.md`) y los commits visibles del repositorio reflejan exactamente la narrativa del notebook:

```
db810df feat: algoritmo de Channel Hopping (auto-recuperación) en nodo esclavo ESP-NOW
9ed9a17 feat: Arquitectura Gateway IoT, unificación UI y estandarización BD
cfe5356 feat: Migración a servidor universitario HTTPS y Backend PHP
bbbedb2 feat: integración de ThingSpeak y Portal Cautivo (WiFiManager)
```

| Capa | Documenta | Código ejecuta |
|---|---|---|
| README / commits | "API REST PHP sobre HTTPS bypass al firewall del 3306" | `requests.get("https://...api_sensores.php")` ✅ |
| Esquema BD | `lecturas_sensores(id, dispositivo, valor, fecha)` | Filtro y casting sobre esas 4 columnas ✅ |
| Firmware ESP32 | Envía DHT22 + MQ135 + Humedad Tierra | Filtro `disp_validos` los acepta a todos ✅ |

**Veredicto:** narrativa **técnicamente honesta y verificable**. No hay discrepancia entre lo declarado en commits/README y lo que muestra el notebook.

### 4.3 Compatibilidad Colab vs Windows local

- ✅ El archivo entregado (`Grupo2_Evaluacion_Sumativa_Colab.ipynb`) es la variante **Colab** (con `!pip install` al inicio, sin `findspark`/`winutils`).
- ✅ Existe variante paralela `Grupo2_Sumativa_LOCAL.ipynb` para desarrollo en Windows con `venv`, `findspark` y `winutils.exe` automatizado — **no debe entregarse**.
- ⚠️ Confirmar antes de la entrega que el archivo subido al evaluador es **`_Colab.ipynb`** (no la variante LOCAL ni la `Grupo2_Evaluacion_Sumativa.ipynb` heredada).

### 4.4 Seguridad — credenciales en código

| Credencial | Estado |
|---|---|
| `AQICN_TOKEN` | `<PLACEHOLDER_AQICN_TOKEN>` ✅ — sin filtración |
| `MONGO_URI` | `<PLACEHOLDER_MONGO_URI>` ✅ — sin filtración |
| MySQL user/password | **No aplican** — la ingest se hace por API REST sin credenciales en el cliente ✅ |
| URL de la API REST | Pública, sin token → ✅ aceptable para entrega académica; en producción se recomendaría JWT + rate-limiting |

---

## 5. Plan de Acción Pre-Entrega (Opcional — No Bloqueante)

| # | Acción | Tiempo estimado | Beneficio |
|---|---|---|---|
| 1 | Añadir `fuente_origen` a `df_aqicn` (1 línea) | 1 min | Simetría auditiva |
| 2 | Centralizar imports en una celda Fase 0 | 5 min | Limpieza de código |
| 3 | Renombrar vista `tabla_mysql_dht → tabla_mysql_iot` | 5 min | Naming preciso |
| 4 | Añadir vista pivotada `tabla_mysql_dht_pivot` (literal de la rúbrica) | 5 min | Robustez ante evaluador estricto |
| 5 | Añadir comentario explicativo del `coalesce(1)` | 1 min | Defensa anti-anti-patrón |
| 6 | Añadir consulta extra C11 con humedad de tierra | 5 min | Aprovecha sensor IoT actualmente inerte |

**Aplicando estas 6 mejoras, la nota proyectada pasa de ~98% (Excelente borderline) a ~100% (Excelente sin reservas).**

---

## 6. Resumen Ejecutivo

| Dimensión | Score | Comentario |
|---|---|---|
| Estructura del notebook (markdown, fases, nomenclatura) | 10/10 | Profesional, fases numeradas, justificaciones presentes |
| Cumplimiento literal de la rúbrica del docx | 9.5/10 | Todas las consultas funcionales; única reserva: nombres `temperatura_interior`/`humedad` no se exponen como columnas pivotadas |
| Calidad de datos (casting, fallbacks, esquema) | 9.5/10 | Tipado OK en 4/4; `fuente_origen` en 3/4; colisión de `fecha` resuelta |
| Optimización Spark (SQL idiomático) | 9/10 | `coalesce(1)` justificable; CAST repetido eliminado; sin JOINs redundantes |
| Coherencia infra ↔ código ↔ README ↔ commits | 10/10 | Bypass HTTPS bien documentado y ejecutado |
| Seguridad de credenciales | 10/10 | Placeholders sin filtración |
| **Total ponderado** | **98%** | **Excelente — apto para entrega final.** |

---

## 7. Trazabilidad — Mapeo Inverso (rúbrica → celda)

| ID rúbrica | Sección notebook | Celdas |
|---|---|---|
| Fase 0 — Setup Spark | Fase 0 | 1, 2, 3 |
| ID3.1 — Ingesta 4 fuentes | Fase 1 (1.1 a 1.4) | 4–12 |
| ID4.2 — Transformaciones (`altura_metros`, `condicion_clima`) | Fase 2 (2.1, 2.2) | 13–17 |
| Vistas temporales | Fase 3 | 18, 19 |
| RA3 — Integración (`df_unificado` + justificación) | Fase 4 | 20, 21, 22 |
| ID4.1 — Consultas C1–C8 | Fase 5 | 23–39 |
| Valor agregado IoT (C9, C10) | Fase 6 | 40–44 |
| Cierre | — | 45 |

---

## 8. Diferencial vs Auditoría Anterior

| Bug crítico previo | Estado en esta auditoría |
|---|---|
| 🛑 C2 retorna NULL (`DAY=8` fuera de rango) | ✅ **Resuelto** — `DAY=1` alineado al slicing |
| 🛑 C8 cardinalidad 666/667 (vista sin slice) | ✅ **Resuelto** — `pdf_iot.head(20)` antes de crear el DataFrame |
| 🛑 C9 `Local_MQ135` ausente | ✅ **Resuelto** — filtro `LIKE '%MQ135%'` tolerante |
| 🛑 C10 join roto + colisión `fecha` | ✅ **Resuelto** — `fecha_om`/`fecha_iot` aliasadas + reuso de `tabla_unificada` |
| ⚠️ Variable muerta `df_dht_limpio` | ✅ **Eliminada** |
| ⚠️ `CAST(valor AS DOUBLE)` repetido | ✅ **Centralizado** en ingest |
| ⚠️ `GROUP BY usuario` redundante en C6 | ✅ **Reescrito** sin GROUP BY |
| ⚠️ `spark.jars.packages` código muerto | ✅ **Purgado** |

---

*Auditoría generada por Senior Data Engineer — listo para entrega final del 20% de la nota TOP 3 Big Data.*
*Documento de referencia: `Evaluación Sumativa.docx` (rúbrica oficial).*
