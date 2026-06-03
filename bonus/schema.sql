DROP TABLE IF EXISTS lugares CASCADE;

CREATE TABLE lugares (
    id          SERIAL       PRIMARY KEY,
    nombre      VARCHAR(100) NOT NULL,
    email       VARCHAR(255) NOT NULL,
    categoria   VARCHAR(50)  NOT NULL,
    creado_at   TIMESTAMP    NOT NULL DEFAULT NOW(),
    ubicacion   POINT        NOT NULL,
    descripcion TEXT
);

CREATE INDEX idx_lugares_creado_btree    ON lugares USING btree (creado_at);
CREATE INDEX idx_lugares_email_hash      ON lugares USING hash  (email);
CREATE INDEX idx_lugares_ubicacion_gist  ON lugares USING gist  (ubicacion);
