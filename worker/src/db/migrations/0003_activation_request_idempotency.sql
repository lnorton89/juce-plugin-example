-- Migration 0003: Activation request replay tracking

-- up: migration forward

CREATE TABLE IF NOT EXISTS activation_request_idempotency (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    request_id TEXT NOT NULL UNIQUE,
    route TEXT NOT NULL,
    license_key_hash TEXT,
    machine_id_hash TEXT,
    requested_at TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'accepted'
        CHECK(status IN ('accepted', 'replay_detected', 'rate_limited', 'invalid_request', 'failed'))
);

CREATE INDEX IF NOT EXISTS idx_activation_request_idempotency_request_id
    ON activation_request_idempotency(request_id);

-- down: migration rollback

DROP TABLE IF EXISTS activation_request_idempotency;

