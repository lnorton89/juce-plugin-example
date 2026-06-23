-- Migration 0001: Create license, activation, webhook-idempotency, and audit tables

-- up: migration forward

CREATE TABLE IF NOT EXISTS licenses (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    lemon_order_id TEXT NOT NULL UNIQUE,
    license_key TEXT NOT NULL UNIQUE,
    product_id TEXT NOT NULL,
    variant_id TEXT NOT NULL,
    store_id TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active', 'expired', 'refunded', 'disabled', 'pending')),
    customer_name TEXT,
    customer_email TEXT,
    activated_count INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS activations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    license_id INTEGER NOT NULL REFERENCES licenses(id) ON DELETE CASCADE,
    machine_identifier TEXT NOT NULL,
    activated_at TEXT NOT NULL DEFAULT (datetime('now')),
    last_validated_at TEXT NOT NULL DEFAULT (datetime('now')),
    deactivated_at TEXT,
    is_active INTEGER NOT NULL DEFAULT 1 CHECK(is_active IN (0, 1)),
    UNIQUE(license_id, machine_identifier)
);

CREATE INDEX IF NOT EXISTS idx_activations_license_active
    ON activations(license_id, is_active)
    WHERE is_active = 1;

CREATE TABLE IF NOT EXISTS webhook_idempotency (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id TEXT NOT NULL UNIQUE,
    event_type TEXT NOT NULL,
    event_created_at TEXT NOT NULL,
    processed_at TEXT NOT NULL DEFAULT (datetime('now')),
    status TEXT NOT NULL DEFAULT 'completed' CHECK(status IN ('completed', 'failed', 'skipped')),
    result_summary TEXT
);

CREATE INDEX IF NOT EXISTS idx_webhook_idempotency_event_id
    ON webhook_idempotency(event_id);

CREATE TABLE IF NOT EXISTS audit_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    action TEXT NOT NULL,
    actor TEXT NOT NULL,
    resource_type TEXT NOT NULL,
    resource_id TEXT,
    details TEXT,
    ip_address TEXT,
    timestamp TEXT NOT NULL DEFAULT (datetime('now')),
    request_id TEXT
);

CREATE INDEX IF NOT EXISTS idx_audit_log_timestamp ON audit_log(timestamp);
CREATE INDEX IF NOT EXISTS idx_audit_log_action ON audit_log(action);

-- down: migration rollback

DROP TABLE IF EXISTS audit_log;
DROP TABLE IF EXISTS webhook_idempotency;
DROP TABLE IF EXISTS activations;
DROP TABLE IF EXISTS licenses;
