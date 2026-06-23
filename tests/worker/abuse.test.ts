import { describe, it, expect, beforeEach } from 'vitest';
import { handleRequest } from '../../worker/src/routes';
import { Repository } from '../../worker/src/db/repository';
import { buildActivationRateLimitKeys } from '../../worker/src/api/rateLimit';
import { createMockEnv, MockD1Database } from './setup';
import fixture from './fixtures/entitlement-v1.json';

function createCapturingCtx(): ExecutionContext & { promises: Promise<unknown>[] } {
  const promises: Promise<unknown>[] = [];
  return {
    promises,
    waitUntil: (promise: Promise<unknown>) => {
      promises.push(promise);
    },
    passThroughOnException: () => {},
  };
}

function activationRequest(
  path: string,
  overrides?: Partial<{
    licenseKey: string;
    machineId: string;
    requestId: string;
    timestamp: string;
  }>
): Request {
  return new Request(`https://api.lumascope.example.com${path}`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'CF-Connecting-IP': '203.0.113.42',
      'User-Agent': 'LumaScope/1.0',
    },
    body: JSON.stringify({
      licenseKey: 'LICENSE-ABUSE-001',
      machineId: 'machine_abuse_001',
      requestId: `req_${crypto.randomUUID()}`,
      timestamp: new Date().toISOString(),
      appVersion: '1.0.0',
      ...overrides,
    }),
  });
}

describe('activation API abuse controls', () => {
  let mockDb: MockD1Database;
  let env: Record<string, any>;
  let repo: Repository;

  beforeEach(async () => {
    mockDb = new MockD1Database();
    env = createMockEnv({
      ACTIVATION_DB: mockDb,
      SIGNING_PRIVATE_KEY: fixture.privateKeyPkcs8,
      SIGNING_KEY_ID: fixture.input.kid,
      SIGNING_PUBLIC_KEYS: JSON.stringify(fixture.publicKeyRing),
    });
    repo = new Repository(mockDb as unknown as D1Database);
    await repo.upsertFromWebhook({
      lemon_order_id: 'order_abuse_001',
      license_key: 'LICENSE-ABUSE-001',
      product_id: 'prod_1',
      variant_id: 'var_1',
      store_id: 'store_1',
      status: 'active',
    });
  });

  it('builds layered rate-limit keys without raw license or machine identifiers', async () => {
    const keys = await buildActivationRateLimitKeys(
      '/api/v1/activate',
      activationRequest('/api/v1/activate'),
      {
        licenseKey: 'LICENSE-ABUSE-001',
        machineId: 'machine_abuse_001',
        requestId: 'req_12345678',
        timestamp: new Date().toISOString(),
      }
    );

    expect(keys).toHaveLength(3);
    expect(keys.join('\n')).toContain('/api/v1/activate:ip:203.0.113.42');
    expect(keys.join('\n')).toContain(':license:sha256:');
    expect(keys.join('\n')).toContain(':machine:sha256:');
    expect(keys.join('\n')).not.toContain('LICENSE-ABUSE-001');
    expect(keys.join('\n')).not.toContain('machine_abuse_001');
  });

  it('returns rate_limited before activation state mutation when binding rejects a key', async () => {
    const seenKeys: string[] = [];
    env.ACTIVATION_RATE_LIMIT = {
      limit: async ({ key }: { key: string }) => {
        seenKeys.push(key);
        return { success: false };
      },
    };
    const ctx = createCapturingCtx();

    const response = await handleRequest(
      activationRequest('/api/v1/activate'),
      env,
      ctx
    );
    await Promise.all(ctx.promises);

    expect(response.status).toBe(429);
    expect(((await response.json()) as Record<string, any>).error.code).toBe('rate_limited');
    expect(mockDb.activations).toHaveLength(0);
    expect(mockDb.activation_request_idempotency[0].status).toBe('rate_limited');
    expect(seenKeys.join('\n')).not.toContain('LICENSE-ABUSE-001');
  });

  it('returns replay_detected for repeated request IDs before mutating twice', async () => {
    const requestId = `req_${crypto.randomUUID()}`;
    const ctx1 = createCapturingCtx();
    const first = await handleRequest(
      activationRequest('/api/v1/activate', { requestId }),
      env,
      ctx1
    );
    await Promise.all(ctx1.promises);

    const ctx2 = createCapturingCtx();
    const second = await handleRequest(
      activationRequest('/api/v1/activate', { requestId }),
      env,
      ctx2
    );
    await Promise.all(ctx2.promises);

    expect(first.status).toBe(200);
    expect(second.status).toBe(409);
    expect(((await second.json()) as Record<string, any>).error.code).toBe('replay_detected');
    expect(mockDb.activations).toHaveLength(1);
  });

  it('writes structured redacted audit records for success and malformed requests', async () => {
    const ctx = createCapturingCtx();
    const success = await handleRequest(
      activationRequest('/api/v1/activate'),
      env,
      ctx
    );
    await Promise.all(ctx.promises);
    expect(success.status).toBe(200);

    const badCtx = createCapturingCtx();
    const malformed = await handleRequest(
      activationRequest('/api/v1/validate', {
        machineId: '',
        requestId: 'bad',
      }),
      env,
      badCtx
    );
    await Promise.all(badCtx.promises);
    expect(malformed.status).toBe(400);

    expect(mockDb.audit_log.length).toBeGreaterThanOrEqual(2);
    const serialized = JSON.stringify(mockDb.audit_log);
    expect(serialized).toContain('activation_activate');
    expect(serialized).toContain('activation_validate');
    expect(serialized).toContain('licenseKeyHash');
    expect(serialized).toContain('machineIdHash');
    expect(serialized).not.toContain('LICENSE-ABUSE-001');
    expect(serialized).not.toContain('machine_abuse_001');
    expect(serialized).not.toContain('"licenseKey"');
    expect(serialized).not.toContain('"machineId"');
    expect(serialized).not.toContain('SIGNING_PRIVATE_KEY');
  });
});
