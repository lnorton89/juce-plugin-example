import { describe, it, expect, beforeEach } from 'vitest';
import { Repository } from '../../worker/src/db/repository';
import type { License, Activation, WebhookIdempotency, AuditLogEntry } from '../../worker/src/db/schema';
import { MockD1Database } from './setup';

describe('Repository', () => {
  let mockDb: MockD1Database;
  let repo: Repository;

  beforeEach(() => {
    mockDb = new MockD1Database();
    repo = new Repository(mockDb as unknown as D1Database);
  });

  // ── License Operations ──────────────────────────────────────────

  describe('license operations', () => {
    it('should upsert a new license record', async () => {
      const license = await repo.upsertFromWebhook({
        lemon_order_id: 'order_001',
        license_key: 'LICENSE-ABC-123',
        product_id: 'prod_1',
        variant_id: 'var_1',
        store_id: 'store_1',
        status: 'active',
        customer_name: 'Alice',
        customer_email: 'alice@example.com',
      });

      expect(license.lemon_order_id).toBe('order_001');
      expect(license.license_key).toBe('LICENSE-ABC-123');
      expect(license.status).toBe('active');
      expect(mockDb.licenses.length).toBe(1);
    });

    it('should update an existing license on upsert', async () => {
      // Insert initial
      await repo.upsertFromWebhook({
        lemon_order_id: 'order_002',
        license_key: 'LICENSE-DEF-456',
        product_id: 'prod_1',
        variant_id: 'var_1',
        store_id: 'store_1',
        status: 'active',
        customer_name: 'Bob',
        customer_email: 'bob@example.com',
      });

      // Upsert with updated status
      const updated = await repo.upsertFromWebhook({
        lemon_order_id: 'order_002',
        license_key: 'LICENSE-DEF-456',
        product_id: 'prod_1',
        variant_id: 'var_1',
        store_id: 'store_1',
        status: 'expired',
        customer_name: 'Bob',
        customer_email: 'bob@example.com',
      });

      expect(updated.status).toBe('expired');
      // Only one record in the table
      expect(mockDb.licenses.length).toBe(1);
    });

    it('should find a license by order ID', async () => {
      await repo.upsertFromWebhook({
        lemon_order_id: 'order_003',
        license_key: 'LICENSE-GHI-789',
        product_id: 'prod_1',
        variant_id: 'var_1',
        store_id: 'store_1',
        status: 'active',
      });

      const found = await repo.findByOrderId('order_003');
      expect(found).not.toBeNull();
      expect(found!.license_key).toBe('LICENSE-GHI-789');
    });

    it('should return null when finding non-existent order ID', async () => {
      const result = await repo.findByOrderId('non_existent');
      expect(result).toBeNull();
    });

    it('should find a license by license key', async () => {
      await repo.upsertFromWebhook({
        lemon_order_id: 'order_004',
        license_key: 'LICENSE-JKL-012',
        product_id: 'prod_1',
        variant_id: 'var_1',
        store_id: 'store_1',
        status: 'active',
      });

      const found = await repo.findByLicenseKey('LICENSE-JKL-012');
      expect(found).not.toBeNull();
      expect(found!.lemon_order_id).toBe('order_004');
    });

    it('should return null when finding non-existent license key', async () => {
      const result = await repo.findByLicenseKey('NONEXISTENT');
      expect(result).toBeNull();
    });
  });

  // ── Idempotency Operations ──────────────────────────────────────

  describe('idempotency operations', () => {
    it('should return null for unknown event_id', async () => {
      const result = await repo.findIdempotency('unknown_event');
      expect(result).toBeNull();
    });

    it('should mark and find idempotency records', async () => {
      await repo.markIdempotency(
        'evt_001',
        'order_created',
        '2026-06-23T12:00:00Z',
        'completed'
      );

      const found = await repo.findIdempotency('evt_001');
      expect(found).not.toBeNull();
      expect(found!.event_id).toBe('evt_001');
      expect(found!.event_type).toBe('order_created');
      expect(found!.status).toBe('completed');
    });

    it('should not error when marking same event_id twice', async () => {
      await repo.markIdempotency('evt_002', 'order_created', '2026-06-23T12:00:00Z', 'completed');

      // Second mark with different status should not overwrite (ON CONFLICT DO NOTHING)
      await repo.markIdempotency('evt_002', 'order_created', '2026-06-23T12:00:00Z', 'failed');

      const found = await repo.findIdempotency('evt_002');
      expect(found).not.toBeNull();
      expect(found!.status).toBe('completed');
    });
  });

  // ── Audit Operations ────────────────────────────────────────────

  describe('audit operations', () => {
    it('should write an audit log entry', async () => {
      await repo.writeAudit(
        'test_action',
        'test_actor',
        'test_resource',
        'res_001',
        'test details',
        '127.0.0.1',
        'req_001'
      );

      expect(mockDb.audit_log.length).toBe(1);
      expect(mockDb.audit_log[0].action).toBe('test_action');
      expect(mockDb.audit_log[0].actor).toBe('test_actor');
      expect(mockDb.audit_log[0].resource_type).toBe('test_resource');
      expect(mockDb.audit_log[0].resource_id).toBe('res_001');
      expect(mockDb.audit_log[0].details).toBe('test details');
      expect(mockDb.audit_log[0].ip_address).toBe('127.0.0.1');
      expect(mockDb.audit_log[0].request_id).toBe('req_001');
    });

    it('should write multiple audit entries', async () => {
      await repo.writeAudit('action_1', 'actor', 'resource');
      await repo.writeAudit('action_2', 'actor', 'resource');

      expect(mockDb.audit_log.length).toBe(2);
    });
  });

  // ── Activation Operations (Stubs for Phase 5) ──────────────────

  describe('activation operations (stubs)', () => {
    it('should return empty list when no activations exist', async () => {
      const activations = await repo.findActiveActivations(1);
      expect(activations).toEqual([]);
    });

    it('should return only active activations for a license', async () => {
      // Manually insert test data into mock
      mockDb.activations.push({
        id: 1,
        license_id: 1,
        machine_identifier: 'machine_001',
        activated_at: '2026-06-23T12:00:00Z',
        last_validated_at: '2026-06-23T12:00:00Z',
        deactivated_at: null,
        is_active: 1,
      });
      mockDb.activations.push({
        id: 2,
        license_id: 1,
        machine_identifier: 'machine_002',
        activated_at: '2026-06-23T12:00:00Z',
        last_validated_at: '2026-06-23T12:00:00Z',
        deactivated_at: '2026-06-23T13:00:00Z',
        is_active: 0,
      });
      mockDb.activations.push({
        id: 3,
        license_id: 2,
        machine_identifier: 'machine_003',
        activated_at: '2026-06-23T12:00:00Z',
        last_validated_at: '2026-06-23T12:00:00Z',
        deactivated_at: null,
        is_active: 1,
      });

      const active = await repo.findActiveActivations(1);
      expect(active).toHaveLength(1);
      expect(active[0].machine_identifier).toBe('machine_001');
      expect(active[0].is_active).toBeTruthy();
    });
  });

  describe('activation policy operations', () => {
    async function insertActiveLicense(): Promise<number> {
      const license = await repo.upsertFromWebhook({
        lemon_order_id: 'order_policy_001',
        license_key: 'LICENSE-POLICY-001',
        product_id: 'prod_1',
        variant_id: 'var_1',
        store_id: 'store_1',
        status: 'active',
      });
      return license.id;
    }

    it('activates a machine when no active activation exists', async () => {
      const licenseId = await insertActiveLicense();

      const result = await repo.activateMachine(
        licenseId,
        'machine_001',
        '2026-06-23T12:00:00.000Z'
      );

      expect(result.outcome).toBe('activated');
      expect(result.activation?.machine_identifier).toBe('machine_001');
      expect(result.activation?.is_active).toBe(1);
      expect(mockDb.activations).toHaveLength(1);
    });

    it('refreshes the same active machine idempotently', async () => {
      const licenseId = await insertActiveLicense();
      await repo.activateMachine(licenseId, 'machine_001', '2026-06-23T12:00:00.000Z');

      const result = await repo.activateMachine(
        licenseId,
        'machine_001',
        '2026-06-23T12:05:00.000Z'
      );

      expect(result.outcome).toBe('refreshed');
      expect(result.activation?.machine_identifier).toBe('machine_001');
      expect(result.activation?.last_validated_at).toBe('2026-06-23T12:05:00.000Z');
      expect(mockDb.activations).toHaveLength(1);
    });

    it('rejects a different active machine', async () => {
      const licenseId = await insertActiveLicense();
      await repo.activateMachine(licenseId, 'machine_001', '2026-06-23T12:00:00.000Z');

      const result = await repo.activateMachine(
        licenseId,
        'machine_002',
        '2026-06-23T12:05:00.000Z'
      );

      expect(result.outcome).toBe('activation_limit_reached');
      expect(result.activation?.machine_identifier).toBe('machine_001');
      expect(mockDb.activations).toHaveLength(1);
    });

    it('treats unique-index activation collisions as activation limit failures', async () => {
      const licenseId = await insertActiveLicense();
      mockDb.activationInsertRaceWinner = {
        id: 99,
        license_id: licenseId,
        machine_identifier: 'machine_race_winner',
        activated_at: '2026-06-23T12:00:00.000Z',
        last_validated_at: '2026-06-23T12:00:00.000Z',
        deactivated_at: null,
        is_active: 0,
      };
      mockDb.rejectNextActivationInsert = true;
      mockDb.activationInsertRaceWinner.is_active = 1;

      const result = await repo.activateMachine(
        licenseId,
        'machine_race_loser',
        '2026-06-23T12:05:00.000Z'
      );

      expect(result.outcome).toBe('activation_limit_reached');
      expect(result.activation?.machine_identifier).toBe('machine_race_winner');
    });

    it('validates only the matching active machine', async () => {
      const licenseId = await insertActiveLicense();
      await repo.activateMachine(licenseId, 'machine_001', '2026-06-23T12:00:00.000Z');

      const valid = await repo.validateMachine(
        licenseId,
        'machine_001',
        '2026-06-23T12:10:00.000Z'
      );
      const mismatch = await repo.validateMachine(
        licenseId,
        'machine_002',
        '2026-06-23T12:10:00.000Z'
      );

      expect(valid.outcome).toBe('refreshed');
      expect(valid.activation?.last_validated_at).toBe('2026-06-23T12:10:00.000Z');
      expect(mismatch.outcome).toBe('machine_mismatch');
    });

    it('deactivates only the matching active machine and allows a new machine later', async () => {
      const licenseId = await insertActiveLicense();
      await repo.activateMachine(licenseId, 'machine_001', '2026-06-23T12:00:00.000Z');

      const wrongMachine = await repo.deactivateMachine(
        licenseId,
        'machine_002',
        '2026-06-23T12:15:00.000Z'
      );
      expect(wrongMachine.outcome).toBe('machine_mismatch');

      const deactivated = await repo.deactivateMachine(
        licenseId,
        'machine_001',
        '2026-06-23T12:20:00.000Z'
      );
      expect(deactivated.outcome).toBe('deactivated');
      expect(deactivated.activation?.is_active).toBe(0);

      const missing = await repo.validateMachine(
        licenseId,
        'machine_001',
        '2026-06-23T12:25:00.000Z'
      );
      expect(missing.outcome).toBe('activation_not_found');

      const newMachine = await repo.activateMachine(
        licenseId,
        'machine_002',
        '2026-06-23T12:30:00.000Z'
      );
      expect(newMachine.outcome).toBe('activated');
      expect(newMachine.activation?.machine_identifier).toBe('machine_002');
    });
  });
});
