import type {
  License,
  Activation,
  WebhookIdempotency,
  AuditLogEntry,
  LicenseStatus,
  IdempotencyStatus,
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
}
