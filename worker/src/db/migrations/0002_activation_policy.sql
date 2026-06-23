-- Migration 0002: Enforce one active machine per license in D1

-- up: migration forward

CREATE UNIQUE INDEX IF NOT EXISTS idx_activations_one_active_machine
    ON activations(license_id)
    WHERE is_active = 1;

-- down: migration rollback

DROP INDEX IF EXISTS idx_activations_one_active_machine;
