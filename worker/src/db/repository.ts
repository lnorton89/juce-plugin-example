import type {
  License,
  Activation,
  WebhookIdempotency,
  AuditLogEntry,
  LicenseStatus,
  IdempotencyStatus,
  ActivationPolicyResult,
  ActivationRequestIdempotency,
  ActivationRequestStatus,
} from './schema';

export class Repository {
  constructor(private readonly db: D1Database) {}

  async findByOrderId(lemonOrderId: string): Promise<License | null> {
    const row = await this.db
      .prepare('SELECT * FROM licenses WHERE lemon_order_id = ?')
      .bind(lemonOrderId)
      .first<License>();
    return row ?? null;
  }

  async findByLicenseKey(licenseKey: string): Promise<License | null> {
    const row = await this.db
      .prepare('SELECT * FROM licenses WHERE license_key = ?')
      .bind(licenseKey)
      .first<License>();
    return row ?? null;
  }

  async upsertFromWebhook(data: {
    lemon_order_id: string;
    license_key: string;
    product_id: string;
    variant_id: string;
    store_id: string;
    status: LicenseStatus;
    customer_name?: string | null;
    customer_email?: string | null;
  }): Promise<License> {
    await this.db
      .prepare(
        `INSERT INTO licenses (lemon_order_id, license_key, product_id, variant_id, store_id, status, customer_name, customer_email, updated_at)
         VALUES (?, ?, ?, ?, ?, ?, ?, ?, datetime('now'))
         ON CONFLICT(lemon_order_id) DO UPDATE SET
           license_key = excluded.license_key,
           product_id = excluded.product_id,
           variant_id = excluded.variant_id,
           store_id = excluded.store_id,
           status = excluded.status,
           customer_name = excluded.customer_name,
           customer_email = excluded.customer_email,
           updated_at = excluded.updated_at`
      )
      .bind(
        data.lemon_order_id,
        data.license_key,
        data.product_id,
        data.variant_id,
        data.store_id,
        data.status,
        data.customer_name ?? null,
        data.customer_email ?? null
      )
      .run();

    const license = await this.findByOrderId(data.lemon_order_id);
    if (license === null) {
      throw new Error('Failed to retrieve license after upsert');
    }
    return license;
  }

  async findIdempotency(eventId: string): Promise<WebhookIdempotency | null> {
    const row = await this.db
      .prepare('SELECT * FROM webhook_idempotency WHERE event_id = ?')
      .bind(eventId)
      .first<WebhookIdempotency>();
    return row ?? null;
  }

  async markIdempotency(
    eventId: string,
    eventType: string,
    eventCreatedAt: string,
    status: IdempotencyStatus,
    resultSummary?: string
  ): Promise<void> {
    await this.db
      .prepare(
        `INSERT INTO webhook_idempotency (event_id, event_type, event_created_at, status, result_summary)
         VALUES (?, ?, ?, ?, ?)
         ON CONFLICT(event_id) DO NOTHING`
      )
      .bind(eventId, eventType, eventCreatedAt, status, resultSummary ?? null)
      .run();
  }

  async writeAudit(
    action: string,
    actor: string,
    resourceType: string,
    resourceId?: string,
    details?: string,
    ipAddress?: string,
    requestId?: string
  ): Promise<void> {
    await this.db
      .prepare(
        `INSERT INTO audit_log (action, actor, resource_type, resource_id, details, ip_address, request_id)
         VALUES (?, ?, ?, ?, ?, ?, ?)`
      )
      .bind(
        action,
        actor,
        resourceType,
        resourceId ?? null,
        details ?? null,
        ipAddress ?? null,
        requestId ?? null
      )
      .run();
  }

  async findActiveActivations(licenseId: number): Promise<Activation[]> {
    const result = await this.db
      .prepare('SELECT * FROM activations WHERE license_id = ? AND is_active = 1')
      .bind(licenseId)
      .all<Activation>();
    return result.results;
  }

  async findActivationRequest(
    requestId: string
  ): Promise<ActivationRequestIdempotency | null> {
    const row = await this.db
      .prepare('SELECT * FROM activation_request_idempotency WHERE request_id = ?')
      .bind(requestId)
      .first<ActivationRequestIdempotency>();
    return row ?? null;
  }

  async markActivationRequest(data: {
    requestId: string;
    route: string;
    licenseKeyHash?: string | null;
    machineIdHash?: string | null;
    requestedAt: string;
    status: ActivationRequestStatus;
  }): Promise<void> {
    await this.db
      .prepare(
        `INSERT INTO activation_request_idempotency
           (request_id, route, license_key_hash, machine_id_hash, requested_at, status)
         VALUES (?, ?, ?, ?, ?, ?)
         ON CONFLICT(request_id) DO NOTHING`
      )
      .bind(
        data.requestId,
        data.route,
        data.licenseKeyHash ?? null,
        data.machineIdHash ?? null,
        data.requestedAt,
        data.status
      )
      .run();
  }

  async findActivationByMachine(
    licenseId: number,
    machineIdentifier: string
  ): Promise<Activation | null> {
    const row = await this.db
      .prepare('SELECT * FROM activations WHERE license_id = ? AND machine_identifier = ?')
      .bind(licenseId, machineIdentifier)
      .first<Activation>();
    return row ?? null;
  }

  async activateMachine(
    licenseId: number,
    machineIdentifier: string,
    nowIso: string
  ): Promise<ActivationPolicyResult> {
    const active = await this.findActiveActivations(licenseId);
    const activeMatch = active.find((activation) => activation.machine_identifier === machineIdentifier);
    if (activeMatch !== undefined) {
      await this.refreshActivation(activeMatch.id, nowIso);
      return {
        outcome: 'refreshed',
        activation: await this.findActivationByMachine(licenseId, machineIdentifier),
      };
    }

    if (active.length > 0) {
      return { outcome: 'activation_limit_reached', activation: active[0] };
    }

    const existing = await this.findActivationByMachine(licenseId, machineIdentifier);

    try {
      if (existing !== null) {
        await this.db
          .prepare(
            `UPDATE activations
             SET is_active = 1, deactivated_at = NULL, last_validated_at = ?, activated_at = ?
             WHERE id = ?`
          )
          .bind(nowIso, nowIso, existing.id)
          .run();
      } else {
        await this.db
          .prepare(
            `INSERT INTO activations (license_id, machine_identifier, activated_at, last_validated_at, is_active)
             VALUES (?, ?, ?, ?, 1)`
          )
          .bind(licenseId, machineIdentifier, nowIso, nowIso)
          .run();
      }
    } catch {
      const racedActive = await this.findActiveActivations(licenseId);
      const racedMatch = racedActive.find(
        (activation) => activation.machine_identifier === machineIdentifier
      );
      return {
        outcome: racedMatch !== undefined ? 'refreshed' : 'activation_limit_reached',
        activation: racedMatch ?? racedActive[0] ?? null,
      };
    }

    return {
      outcome: existing === null ? 'activated' : 'refreshed',
      activation: await this.findActivationByMachine(licenseId, machineIdentifier),
    };
  }

  async validateMachine(
    licenseId: number,
    machineIdentifier: string,
    nowIso: string
  ): Promise<ActivationPolicyResult> {
    const active = await this.findActiveActivations(licenseId);
    const activeMatch = active.find((activation) => activation.machine_identifier === machineIdentifier);

    if (activeMatch === undefined) {
      return {
        outcome: active.length > 0 ? 'machine_mismatch' : 'activation_not_found',
        activation: active[0] ?? null,
      };
    }

    await this.refreshActivation(activeMatch.id, nowIso);
    return {
      outcome: 'refreshed',
      activation: await this.findActivationByMachine(licenseId, machineIdentifier),
    };
  }

  async deactivateMachine(
    licenseId: number,
    machineIdentifier: string,
    nowIso: string
  ): Promise<ActivationPolicyResult> {
    const active = await this.findActiveActivations(licenseId);
    const activeMatch = active.find((activation) => activation.machine_identifier === machineIdentifier);

    if (activeMatch === undefined) {
      return {
        outcome: active.length > 0 ? 'machine_mismatch' : 'activation_not_found',
        activation: active[0] ?? null,
      };
    }

    await this.db
      .prepare(
        `UPDATE activations
         SET is_active = 0, deactivated_at = ?, last_validated_at = ?
         WHERE id = ?`
      )
      .bind(nowIso, nowIso, activeMatch.id)
      .run();

    return {
      outcome: 'deactivated',
      activation: await this.findActivationByMachine(licenseId, machineIdentifier),
    };
  }

  private async refreshActivation(activationId: number, nowIso: string): Promise<void> {
    await this.db
      .prepare('UPDATE activations SET last_validated_at = ? WHERE id = ?')
      .bind(nowIso, activationId)
      .run();
  }
}
