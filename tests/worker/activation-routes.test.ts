import { describe, it, expect, beforeEach } from 'vitest';
import { handleRequest } from '../../worker/src/routes';
import { Repository } from '../../worker/src/db/repository';
import { verifyEntitlementSignature } from '../../worker/src/signing/entitlement';
import { createMockEnv, MockD1Database } from './setup';
import fixture from './fixtures/entitlement-v1.json';

function mockCtx(): ExecutionContext {
  return {
    waitUntil: () => {},
    passThroughOnException: () => {},
  };
}

function activationRequest(path: string, machineId: string): Request {
  return new Request(`https://api.lumascope.example.com${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      licenseKey: 'LICENSE-ACTIVE-001',
      machineId,
      requestId: `req_${crypto.randomUUID()}`,
      timestamp: new Date().toISOString(),
      appVersion: '1.0.0',
    }),
  });
}

describe('activation API routes', () => {
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
      lemon_order_id: 'order_active_001',
      license_key: 'LICENSE-ACTIVE-001',
      product_id: 'prod_1',
      variant_id: 'var_1',
      store_id: 'store_1',
      status: 'active',
    });
  });

  it('activates a valid license and returns a verifiable signed entitlement', async () => {
    const response = await handleRequest(
      activationRequest('/api/v1/activate', 'machine_001'),
      env,
      mockCtx()
    );

    expect(response.status).toBe(200);
    const body = (await response.json()) as Record<string, any>;
    expect(body.activationId).toBe('act_1');
    expect(body.tokenType).toBe('lumascope-entitlement-v1');
    expect(body.entitlementToken.payload.machineId).toBe('machine_001');
    expect(body.entitlementToken.payload.kid).toBe(fixture.input.kid);
    expect(JSON.stringify(body.entitlementToken.payload)).not.toMatch(/LICENSE-|customer|email/i);
    expect(
      await verifyEntitlementSignature(body.entitlementToken, fixture.publicKeySpki)
    ).toBe(true);
  });

  it('returns idempotent success for same-machine activation retry', async () => {
    const first = await handleRequest(
      activationRequest('/api/v1/activate', 'machine_001'),
      env,
      mockCtx()
    );
    const second = await handleRequest(
      activationRequest('/api/v1/activate', 'machine_001'),
      env,
      mockCtx()
    );

    expect(first.status).toBe(200);
    expect(second.status).toBe(200);
    expect(mockDb.activations).toHaveLength(1);
    const body = (await second.json()) as Record<string, any>;
    expect(body.activationId).toBe('act_1');
  });

  it('rejects different-machine activation while another active machine exists', async () => {
    await handleRequest(activationRequest('/api/v1/activate', 'machine_001'), env, mockCtx());

    const response = await handleRequest(
      activationRequest('/api/v1/activate', 'machine_002'),
      env,
      mockCtx()
    );

    expect(response.status).toBe(409);
    const body = (await response.json()) as Record<string, any>;
    expect(body.error.code).toBe('activation_limit_reached');
    expect(mockDb.activations).toHaveLength(1);
  });

  it('rejects inactive and missing licenses with generic stable errors', async () => {
    await repo.upsertFromWebhook({
      lemon_order_id: 'order_inactive_001',
      license_key: 'LICENSE-INACTIVE-001',
      product_id: 'prod_1',
      variant_id: 'var_1',
      store_id: 'store_1',
      status: 'expired',
    });

    const inactive = new Request('https://api.lumascope.example.com/api/v1/activate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        licenseKey: 'LICENSE-INACTIVE-001',
        machineId: 'machine_001',
        requestId: `req_${crypto.randomUUID()}`,
        timestamp: new Date().toISOString(),
      }),
    });

    const missing = new Request('https://api.lumascope.example.com/api/v1/activate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        licenseKey: 'LICENSE-MISSING-001',
        machineId: 'machine_001',
        requestId: `req_${crypto.randomUUID()}`,
        timestamp: new Date().toISOString(),
      }),
    });

    const inactiveResponse = await handleRequest(inactive, env, mockCtx());
    const missingResponse = await handleRequest(missing, env, mockCtx());

    expect(inactiveResponse.status).toBe(403);
    expect(((await inactiveResponse.json()) as Record<string, any>).error.code).toBe('license_inactive');
    expect(missingResponse.status).toBe(404);
    expect(((await missingResponse.json()) as Record<string, any>).error.code).toBe('license_not_found');
  });

  it('validates an existing active activation but does not recreate missing activations', async () => {
    const missing = await handleRequest(
      activationRequest('/api/v1/validate', 'machine_001'),
      env,
      mockCtx()
    );
    expect(missing.status).toBe(404);
    expect(((await missing.json()) as Record<string, any>).error.code).toBe('activation_not_found');

    await handleRequest(activationRequest('/api/v1/activate', 'machine_001'), env, mockCtx());
    const valid = await handleRequest(
      activationRequest('/api/v1/validate', 'machine_001'),
      env,
      mockCtx()
    );
    expect(valid.status).toBe(200);

    const mismatch = await handleRequest(
      activationRequest('/api/v1/validate', 'machine_002'),
      env,
      mockCtx()
    );
    expect(mismatch.status).toBe(403);
    expect(((await mismatch.json()) as Record<string, any>).error.code).toBe('machine_mismatch');
  });

  it('deactivates only the matching machine and allows transfer to another machine', async () => {
    await handleRequest(activationRequest('/api/v1/activate', 'machine_001'), env, mockCtx());

    const wrongMachine = await handleRequest(
      activationRequest('/api/v1/deactivate', 'machine_002'),
      env,
      mockCtx()
    );
    expect(wrongMachine.status).toBe(403);
    expect(((await wrongMachine.json()) as Record<string, any>).error.code).toBe('machine_mismatch');

    const deactivated = await handleRequest(
      activationRequest('/api/v1/deactivate', 'machine_001'),
      env,
      mockCtx()
    );
    expect(deactivated.status).toBe(200);
    expect(((await deactivated.json()) as Record<string, any>).status).toBe('deactivated');

    const validateAfterDeactivate = await handleRequest(
      activationRequest('/api/v1/validate', 'machine_001'),
      env,
      mockCtx()
    );
    expect(validateAfterDeactivate.status).toBe(404);

    const newMachine = await handleRequest(
      activationRequest('/api/v1/activate', 'machine_002'),
      env,
      mockCtx()
    );
    expect(newMachine.status).toBe(200);
    const body = (await newMachine.json()) as Record<string, any>;
    expect(body.entitlementToken.payload.machineId).toBe('machine_002');
  });

  it('rejects malformed request bodies before mutating activation state', async () => {
    const response = await handleRequest(
      new Request('https://api.lumascope.example.com/api/v1/activate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          licenseKey: 'LICENSE-ACTIVE-001',
          machineId: '',
          requestId: 'bad',
          timestamp: new Date().toISOString(),
        }),
      }),
      env,
      mockCtx()
    );

    expect(response.status).toBe(400);
    expect(mockDb.activations).toHaveLength(0);
  });
});
