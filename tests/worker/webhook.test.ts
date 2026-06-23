import { describe, it, expect, beforeEach } from 'vitest';
import {
  verifyLemonSignature,
  parseLemonEvent,
  handleLemonWebhook,
} from '../../worker/src/webhook';
import {
  generateSignature,
  createMockEnv,
  stripTestMetadata,
  MockD1Database,
} from './setup';
import validFixture from './fixtures/lemon-webhook-valid.json';

function mockCtx(): ExecutionContext {
  return {
    waitUntil: () => {},
    passThroughOnException: () => {},
  };
}

async function createSignedRequest(
  body: string,
  secret: string,
  overrides?: Record<string, string>
): Promise<Request> {
  const signature = await generateSignature(body, secret);
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    'X-Signature': signature,
    ...overrides,
  };
  return new Request('https://api.lumascope.example.com/api/webhook/lemon-squeezy', {
    method: 'POST',
    headers,
    body,
  });
}

// ── Signature Verification Tests ──────────────────────────────────

describe('verifyLemonSignature', () => {
  it('should return valid for a correctly signed request', async () => {
    const body = JSON.stringify({ test: 'data' });
    const secret = 'test_secret_123';
    const signature = await generateSignature(body, secret);
    const request = new Request('https://example.com/webhook', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'X-Signature': signature },
      body,
    });

    const result = await verifyLemonSignature(request, secret);
    expect(result.valid).toBe(true);
    expect(result.body).toBe(body);
  });

  it('should return invalid for a wrong signature', async () => {
    const body = JSON.stringify({ test: 'data' });
    const secret = 'test_secret_123';
    const wrongSignature = '0000000000000000000000000000000000000000000000000000000000000000';
    const request = new Request('https://example.com/webhook', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'X-Signature': wrongSignature },
      body,
    });

    const result = await verifyLemonSignature(request, secret);
    expect(result.valid).toBe(false);
  });

  it('should return invalid when X-Signature header is missing', async () => {
    const body = JSON.stringify({ test: 'data' });
    const secret = 'test_secret_123';
    const request = new Request('https://example.com/webhook', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body,
    });

    const result = await verifyLemonSignature(request, secret);
    expect(result.valid).toBe(false);
  });

  it('should return invalid with wrong secret', async () => {
    const body = JSON.stringify({ test: 'data' });
    const correctSecret = 'test_secret_123';
    const wrongSecret = 'wrong_secret_456';
    const signature = await generateSignature(body, correctSecret);
    const request = new Request('https://example.com/webhook', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'X-Signature': signature },
      body,
    });

    const result = await verifyLemonSignature(request, wrongSecret);
    expect(result.valid).toBe(false);
  });
});

// ── Event Parsing Tests ──────────────────────────────────────────

describe('parseLemonEvent', () => {
  it('should parse a valid Lemon webhook event', () => {
    const body = stripTestMetadata(validFixture);
    const event = parseLemonEvent(body);

    expect(event).not.toBeNull();
    expect(event!.eventName).toBe('order_created');
    expect(event!.eventId).toBe('evt_a1b2c3d4-e5f6-7890-abcd-ef1234567890');
    expect(event!.storeId).toBe('store_1');
    expect(event!.productId).toBe('prod_1');
    expect(event!.variantId).toBe('var_1');
    expect(event!.status).toBe('paid');
    expect(event!.customerName).toBe('Jane Doe');
    expect(event!.customerEmail).toBe('jane@example.com');
    expect(event!.licenseKey).toBeNull();
  });

  it('should return null for malformed JSON', () => {
    const event = parseLemonEvent('not valid json');
    expect(event).toBeNull();
  });

  it('should return null for missing required fields', () => {
    const event = parseLemonEvent(JSON.stringify({}));
    expect(event).toBeNull();
  });

  it('should return null for empty event_name', () => {
    const body = JSON.stringify({
      meta: { event_name: '', custom_data: {} },
      data: { id: 'test', type: 'test', attributes: { store_id: 1 } },
    });
    const event = parseLemonEvent(body);
    expect(event).toBeNull();
  });

  it('should return null when product_id or variant_id cannot be resolved', () => {
    const body = JSON.stringify({
      meta: { event_name: 'order_created', custom_data: {} },
      data: {
        id: 'test',
        type: 'orders',
        attributes: {
          store_id: 1,
          status: 'active',
          created_at: '2026-06-23T12:00:00Z',
        },
      },
    });
    const event = parseLemonEvent(body);
    expect(event).toBeNull();
  });
});

// ── Full Webhook Integration Tests ────────────────────────────────

describe('handleLemonWebhook', () => {
  let mockDb: MockD1Database;
  let env: Record<string, any>;

  beforeEach(() => {
    mockDb = new MockD1Database();
    env = createMockEnv({ ACTIVATION_DB: mockDb });
  });

  function makeRecentEventBody(overrides?: Record<string, any>): string {
    const event = {
      meta: {
        event_name: 'order_created',
        custom_data: { store_id: 'store_1', product_id: 'prod_1', variant_id: 'var_1' },
      },
      data: {
        id: 'evt_a1b2c3d4-e5f6-7890-abcd-ef1234567890',
        type: 'orders',
        attributes: {
          store_id: 1,
          status: 'paid',
          user_name: 'Jane Doe',
          user_email: 'jane@example.com',
          first_order_item: { product_id: 1, variant_id: 1 },
          created_at: new Date().toISOString(),
        },
      },
    };
    // Merge overrides
    if (overrides) {
      if (overrides.meta) Object.assign(event.meta, overrides.meta);
      if (overrides.data) Object.assign(event.data, overrides.data);
      if (overrides.attributes) Object.assign(event.data.attributes, overrides.attributes);
    }
    return JSON.stringify(event);
  }

  it('should process a valid webhook and return 200', async () => {
    const body = makeRecentEventBody();
    const request = await createSignedRequest(body, 'test_secret_123');
    const response = await handleLemonWebhook(request, env, mockCtx());

    expect(response.status).toBe(200);
    const json = await response.json() as Record<string, any>;
    expect(json.status).toBe('processed');
    expect(json.event).toBe('order_created');
    expect(json.event_id).toBe('evt_a1b2c3d4-e5f6-7890-abcd-ef1234567890');
  });

  it('should return 200 for duplicate event (idempotency)', async () => {
    const body = makeRecentEventBody();
    const request = await createSignedRequest(body, 'test_secret_123');

    // First call
    const response1 = await handleLemonWebhook(request, env, mockCtx());
    expect(response1.status).toBe(200);

    // Second call with same event
    const request2 = await createSignedRequest(body, 'test_secret_123');
    const response2 = await handleLemonWebhook(request2, env, mockCtx());
    expect(response2.status).toBe(200);

    const json = await response2.json() as Record<string, any>;
    expect(json.status).toBe('processed');

    // Verify only one license record was created
    expect(mockDb.licenses.length).toBe(1);
  });

  it('should return 200 with skipped for unsupported event type', async () => {
    const body = makeRecentEventBody({
      meta: { event_name: 'review_created' },
      data: { id: 'evt_review-0000-0000-0000-000000000001', type: 'reviews' },
      attributes: { reviewer_name: 'Bob Smith', body: 'Great!' },
    });
    const request = await createSignedRequest(body, 'test_secret_123');
    const response = await handleLemonWebhook(request, env, mockCtx());

    expect(response.status).toBe(200);
    const json = await response.json() as Record<string, any>;
    expect(json.status).toBe('skipped');
    expect(json.reason).toBe('unsupported event type');
  });

  it('should return 200 with skipped for unconfigured store/product', async () => {
    // Override env with a different store/product
    const mismatchedEnv = createMockEnv({
      ACTIVATION_DB: mockDb,
      LEMON_STORE_ID: 'store_999',
      LEMON_PRODUCT_ID: 'prod_999',
      LEMON_VARIANT_ID: 'var_999',
    });

    const body = makeRecentEventBody();
    const request = await createSignedRequest(body, 'test_secret_123');
    const response = await handleLemonWebhook(request, mismatchedEnv, mockCtx());

    expect(response.status).toBe(200);
    const json = await response.json() as Record<string, any>;
    expect(json.status).toBe('skipped');
    expect(json.reason).toBe('unconfigured product');
  });

  it('should return 400 for expired event (older than 5 minutes)', async () => {
    // Build an event payload with an old timestamp
    const oldEvent = {
      meta: {
        event_name: 'order_created',
        custom_data: { store_id: 'store_1', product_id: 'prod_1', variant_id: 'var_1' },
      },
      data: {
        id: 'evt_old-0000-0000-0000-000000000001',
        type: 'orders',
        attributes: {
          store_id: 1,
          status: 'paid',
          user_name: 'Old User',
          user_email: 'old@example.com',
          first_order_item: { product_id: 1, variant_id: 1 },
          created_at: '2020-01-01T00:00:00Z', // 6+ years ago
        },
      },
    };
    const body = JSON.stringify(oldEvent);
    const request = await createSignedRequest(body, 'test_secret_123');
    const response = await handleLemonWebhook(request, env, mockCtx());

    expect(response.status).toBe(400);
    const json = await response.json() as Record<string, any>;
    expect(json.error).toBe('Bad Request');
  });

  it('should return 400 for missing Content-Type', async () => {
    const body = makeRecentEventBody();
    const signature = await generateSignature(body, 'test_secret_123');
    const request = new Request('https://api.lumascope.example.com/api/webhook/lemon-squeezy', {
      method: 'POST',
      headers: { 'X-Signature': signature },
      body,
    });

    const response = await handleLemonWebhook(request, env, mockCtx());
    expect(response.status).toBe(400);
  });

  it('should return 413 for oversized body', async () => {
    const body = makeRecentEventBody();
    const signature = await generateSignature(body, 'test_secret_123');
    const request = new Request('https://api.lumascope.example.com/api/webhook/lemon-squeezy', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'X-Signature': signature,
        'Content-Length': '9999999',
      },
      body,
    });

    const response = await handleLemonWebhook(request, env, mockCtx());
    expect(response.status).toBe(413);
  });

  it('should return 401 for invalid signature', async () => {
    const body = makeRecentEventBody();
    const request = new Request('https://api.lumascope.example.com/api/webhook/lemon-squeezy', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'X-Signature': '0000000000000000000000000000000000000000000000000000000000000000',
      },
      body,
    });

    const response = await handleLemonWebhook(request, env, mockCtx());
    expect(response.status).toBe(401);
  });

  it('should handle a subscription_created event', async () => {
    const subEvent = {
      meta: {
        event_name: 'subscription_created',
        custom_data: { store_id: 'store_1', product_id: 'prod_1', variant_id: 'var_1' },
      },
      data: {
        id: 'evt_sub-0000-0000-0000-000000000001',
        type: 'subscriptions',
        attributes: {
          store_id: 1,
          status: 'active',
          user_name: 'Sub User',
          user_email: 'sub@example.com',
          first_subscription_item: { product_id: 1, variant_id: 1 },
          created_at: new Date().toISOString(),
        },
      },
    };
    const body = JSON.stringify(subEvent);
    const request = await createSignedRequest(body, 'test_secret_123');
    const response = await handleLemonWebhook(request, env, mockCtx());

    expect(response.status).toBe(200);
    const json = await response.json() as Record<string, any>;
    expect(json.status).toBe('processed');
    expect(json.event).toBe('subscription_created');
  });

  it('should handle a license_key_created event with license key', async () => {
    const licenseEvent = {
      meta: {
        event_name: 'license_key_created',
        custom_data: { store_id: 'store_1', product_id: 'prod_1', variant_id: 'var_1' },
      },
      data: {
        id: 'evt_lic-0000-0000-0000-000000000001',
        type: 'license-keys',
        attributes: {
          store_id: 1,
          status: 'active',
          license_key: {
            key: 'LICENSE-XXXX-YYYY-ZZZZ',
            product_id: 1,
            variant_id: 1,
            status: 'active',
          },
          created_at: new Date().toISOString(),
        },
      },
    };
    const body = JSON.stringify(licenseEvent);
    const request = await createSignedRequest(body, 'test_secret_123');
    const response = await handleLemonWebhook(request, env, mockCtx());

    expect(response.status).toBe(200);
    const json = await response.json() as Record<string, any>;
    expect(json.status).toBe('processed');
    expect(json.event).toBe('license_key_created');
  });

  it('should handle missing LEMON_VARIANT_ID env var gracefully', async () => {
    const partialEnv = createMockEnv({
      ACTIVATION_DB: mockDb,
      LEMON_STORE_ID: 'store_1',
      LEMON_PRODUCT_ID: 'prod_1',
      LEMON_VARIANT_ID: '', // empty — skip variant check
    });

    const body = makeRecentEventBody();
    const request = await createSignedRequest(body, 'test_secret_123');
    const response = await handleLemonWebhook(request, partialEnv, mockCtx());

    expect(response.status).toBe(200);
    const json = await response.json() as Record<string, any>;
    expect(json.status).toBe('processed');
  });
});
