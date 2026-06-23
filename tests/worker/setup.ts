/**
 * Test setup and mock infrastructure for Worker tests.
 * Provides MockD1Database, generateSignature, and mock env objects.
 */

export async function generateSignature(body: string, secret: string): Promise<string> {
  const encoder = new TextEncoder();
  const key = await crypto.subtle.importKey(
    'raw',
    encoder.encode(secret),
    { name: 'HMAC', hash: 'SHA-256' },
    false,
    ['sign']
  );
  const signature = await crypto.subtle.sign('HMAC', key, encoder.encode(body));
  return arrayBufferToHex(signature);
}

function arrayBufferToHex(buffer: ArrayBuffer): string {
  const bytes = new Uint8Array(buffer);
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, '0'))
    .join('');
}

/**
 * In-memory mock of D1Database for testing Repository operations.
 * Handles the specific SQL patterns used by Repository.ts:
 * - SELECT ... FROM licenses WHERE lemon_order_id = ?
 * - SELECT ... FROM licenses WHERE license_key = ?
 * - INSERT INTO licenses (...) VALUES (...) ON CONFLICT(...) DO UPDATE SET ...
 * - SELECT ... FROM webhook_idempotency WHERE event_id = ?
 * - INSERT INTO webhook_idempotency (...) VALUES (...) ON CONFLICT(...) DO NOTHING
 * - INSERT INTO audit_log (...) VALUES (...)
 * - SELECT ... FROM activations WHERE license_id = ? AND is_active = 1
 * - SELECT ... FROM activations WHERE license_id = ? AND is_active = ?
 */
export class MockD1Database {
  licenses: Record<string, any>[] = [];
  activations: Record<string, any>[] = [];
  webhook_idempotency: Record<string, any>[] = [];
  audit_log: Record<string, any>[] = [];

  private sequences: Record<string, number> = {
    licenses: 1,
    activations: 1,
    webhook_idempotency: 1,
    audit_log: 1,
  };

  prepare(sql: string): MockPreparedStatement {
    return new MockPreparedStatement(sql, this);
  }

  /** Reset all tables. Call between tests for isolation. */
  reset(): void {
    this.licenses = [];
    this.activations = [];
    this.webhook_idempotency = [];
    this.audit_log = [];
    this.sequences = { licenses: 1, activations: 1, webhook_idempotency: 1, audit_log: 1 };
  }
}

class MockPreparedStatement {
  private bindValues: any[] = [];

  constructor(
    private sql: string,
    private db: MockD1Database
  ) {}

  bind(...values: any[]): this {
    this.bindValues = values;
    return this;
  }

  async first<T>(): Promise<T | null> {
    const normalized = this.normalizeSql(this.sql);

    // SELECT * FROM licenses WHERE lemon_order_id = ?
    if (normalized.includes('from licenses') && normalized.includes('lemon_order_id')) {
      const row = this.db.licenses.find(
        (l) => l.lemon_order_id === this.bindValues[0]
      );
      return (row ? { ...row } : null) as T | null;
    }

    // SELECT * FROM licenses WHERE license_key = ?
    if (normalized.includes('from licenses') && normalized.includes('license_key')) {
      const row = this.db.licenses.find(
        (l) => l.license_key === this.bindValues[0]
      );
      return (row ? { ...row } : null) as T | null;
    }

    // SELECT * FROM webhook_idempotency WHERE event_id = ?
    if (normalized.includes('from webhook_idempotency')) {
      const row = this.db.webhook_idempotency.find(
        (w) => w.event_id === this.bindValues[0]
      );
      return (row ? { ...row } : null) as T | null;
    }

    // SELECT * FROM activations WHERE license_id = ? AND is_active = ?
    if (normalized.includes('from activations')) {
      const matches = this.db.activations.filter((a) => {
        return (
          a.license_id === this.bindValues[0] &&
          a.is_active === this.bindValues[1]
        );
      });
      return (matches.length > 0 ? { ...matches[0] } : null) as T | null;
    }

    return null;
  }

  async all<T>(): Promise<D1Result<T>> {
    const normalized = this.normalizeSql(this.sql);

    // SELECT * FROM activations WHERE license_id = ? AND is_active = 1
    if (normalized.includes('from activations')) {
      const rows = this.db.activations.filter((a) => {
        return (
          a.license_id === this.bindValues[0] && a.is_active === 1
        );
      });
      return {
        success: true,
        results: rows.map((r) => ({ ...r })) as T[],
        meta: { duration: 0 },
      };
    }

    return { success: true, results: [], meta: { duration: 0 } };
  }

  async run<T>(): Promise<D1Result<T>> {
    const normalized = this.normalizeSql(this.sql);

    // INSERT INTO licenses
    if (normalized.includes('insert') && normalized.includes('into licenses')) {
      return this.handleInsertLicenses();
    }

    // INSERT INTO webhook_idempotency
    if (normalized.includes('insert') && normalized.includes('into webhook_idempotency')) {
      return this.handleInsertIdempotency();
    }

    // INSERT INTO audit_log
    if (normalized.includes('insert') && normalized.includes('into audit_log')) {
      return this.handleInsertAudit();
    }

    return {
      success: true,
      results: [],
      meta: { duration: 0, changes: 0 },
    };
  }

  private normalizeSql(sql: string): string {
    return sql.replace(/\s+/g, ' ').trim().toLowerCase();
  }

  private handleInsertLicenses(): D1Result<any> {
    // Extract column names from: INSERT INTO licenses (col1, col2, ...)
    const colMatch = this.sql.match(/\(([^)]+)\)\s*VALUES/i);
    if (!colMatch) {
      return {
        success: false,
        results: [],
        meta: { duration: 0 },
        error: 'Could not parse columns',
      };
    }

    const columns = colMatch[1]
      .split(',')
      .map((c) => c.trim().replace(/"/g, '').toLowerCase());

    const row: Record<string, any> = {};
    columns.forEach((col, i) => {
      if (i < this.bindValues.length) {
        row[col] = this.bindValues[i];
      }
    });

    // Handle ON CONFLICT: find existing by lemon_order_id
    const existingIdx = this.db.licenses.findIndex(
      (l) => l.lemon_order_id === row.lemon_order_id
    );

    if (existingIdx >= 0) {
      // Update existing - preserve id, created_at, activated_count
      const existing = this.db.licenses[existingIdx];
      this.db.licenses[existingIdx] = {
        ...existing,
        ...row,
        id: existing.id,
        created_at: existing.created_at,
        activated_count: existing.activated_count,
      };
      return {
        success: true,
        results: [],
        meta: { duration: 0, changes: 1, last_row_id: existing.id },
      };
    }

    row.id = this.db.sequences.licenses++;
    row.created_at = row.created_at || '2026-06-23T12:00:00Z';
    row.updated_at = row.updated_at || '2026-06-23T12:00:00Z';
    row.activated_count = row.activated_count ?? 0;
    this.db.licenses.push(row);

    return {
      success: true,
      results: [],
      meta: { duration: 0, changes: 1, last_row_id: row.id },
    };
  }

  private handleInsertIdempotency(): D1Result<any> {
    const colMatch = this.sql.match(/\(([^)]+)\)\s*VALUES/i);
    if (!colMatch) {
      return {
        success: false,
        results: [],
        meta: { duration: 0 },
        error: 'Could not parse columns',
      };
    }

    const columns = colMatch[1]
      .split(',')
      .map((c) => c.trim().replace(/"/g, '').toLowerCase());

    const row: Record<string, any> = {};
    columns.forEach((col, i) => {
      if (i < this.bindValues.length) {
        row[col] = this.bindValues[i];
      }
    });

    // ON CONFLICT DO NOTHING — check for existing event_id
    const existing = this.db.webhook_idempotency.find(
      (w) => w.event_id === row.event_id
    );
    if (existing) {
      return {
        success: true,
        results: [],
        meta: { duration: 0, changes: 0 },
      };
    }

    row.id = this.db.sequences.webhook_idempotency++;
    row.processed_at = row.processed_at || '2026-06-23T12:00:00Z';
    this.db.webhook_idempotency.push(row);

    return {
      success: true,
      results: [],
      meta: { duration: 0, changes: 1, last_row_id: row.id },
    };
  }

  private handleInsertAudit(): D1Result<any> {
    const colMatch = this.sql.match(/\(([^)]+)\)\s*VALUES/i);
    if (!colMatch) {
      return {
        success: false,
        results: [],
        meta: { duration: 0 },
        error: 'Could not parse columns',
      };
    }

    const columns = colMatch[1]
      .split(',')
      .map((c) => c.trim().replace(/"/g, '').toLowerCase());

    const row: Record<string, any> = {};
    columns.forEach((col, i) => {
      if (i < this.bindValues.length) {
        row[col] = this.bindValues[i];
      }
    });

    row.id = this.db.sequences.audit_log++;
    row.timestamp = row.timestamp || '2026-06-23T12:00:00Z';
    this.db.audit_log.push(row);

    return {
      success: true,
      results: [],
      meta: { duration: 0, changes: 1, last_row_id: row.id },
    };
  }
}

export interface D1Result<T = unknown> {
  results: T[];
  success: boolean;
  meta: {
    duration: number;
    changes?: number;
    last_row_id?: number;
    served_by?: string;
  };
  error?: string;
}

/** Create a mock Env object for webhook testing. */
export function createMockEnv(overrides?: Record<string, string | D1Database>): Record<string, any> {
  return {
    ACTIVATION_DB: new MockD1Database(),
    LEMON_WEBHOOK_SECRET: 'test_secret_123',
    LEMON_STORE_ID: 'store_1',
    LEMON_PRODUCT_ID: 'prod_1',
    LEMON_VARIANT_ID: 'var_1',
    SIGNING_PRIVATE_KEY: '',
    ENVIRONMENT: 'test',
    ...overrides,
  };
}

/** Helper to strip _test metadata from a fixture object. */
export function stripTestMetadata(fixture: Record<string, any>): string {
  const clone = { ...fixture };
  delete clone._test;
  return JSON.stringify(clone);
}

export { MockPreparedStatement };
