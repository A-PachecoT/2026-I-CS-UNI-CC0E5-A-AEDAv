INSERT INTO lugares (nombre, email, categoria, ubicacion, descripcion) VALUES
    ('UNI',         'fc@uni.edu.pe',        'universidad', POINT(-12.0193, -77.0498), 'Facultad de Ciencias'),
    ('PUCP',        'info@pucp.edu.pe',     'universidad', POINT(-12.0719, -77.0792), NULL),
    ('Plaza Mayor', 'plaza@lima.gob.pe',    'turismo',     POINT(-12.0464, -77.0428), 'Centro historico'),
    ('Larcomar',    'contacto@larcomar.pe', 'comercio',    POINT(-12.1318, -77.0303), 'Mall'),
    ('Estadio',     'info@estadio.gob.pe',  'deportes',    POINT(-12.0676, -77.0334), 'Estadio Nacional');

ANALYZE lugares;

-- B-tree
EXPLAIN ANALYZE
SELECT id, nombre, creado_at
FROM lugares
WHERE creado_at > NOW() - INTERVAL '1 hour'
ORDER BY creado_at DESC;

-- Hash
EXPLAIN ANALYZE
SELECT id, nombre, email
FROM lugares
WHERE email = 'fc@uni.edu.pe';

-- GiST contencion espacial
EXPLAIN ANALYZE
SELECT id, nombre, ubicacion
FROM lugares
WHERE ubicacion <@ BOX(POINT(-12.10, -77.08), POINT(-12.00, -77.00));

-- GiST KNN
EXPLAIN ANALYZE
SELECT id, nombre, ubicacion, (ubicacion <-> POINT(-12.05, -77.04)) AS distancia
FROM lugares
ORDER BY ubicacion <-> POINT(-12.05, -77.04)
LIMIT 3;

SELECT indexname, indexdef FROM pg_indexes WHERE tablename = 'lugares' ORDER BY indexname;
