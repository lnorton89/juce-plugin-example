import { describe, it, expect } from 'vitest';
import {
  ACTIVATION_ENDPOINTS,
  type ActivateRequest,
  type ActivateResponse,
} from '../../worker/src/api/contracts';
import {
  ACTIVATION_ERROR_CODES,
  activationErrorResponse,
  activationStatusForError,
  createActivationError,
} from '../../worker/src/api/errors';
import { validateActivationJsonRequest } from '../../worker/src/api/requestValidation';

const now = new Date('2026-06-23T12:00:00.000Z');

function request(body: unknown, init?: RequestInit): Request {
  const method = init?.method ?? 'POST';
  const encodedBody = typeof body === 'string' ? body : JSON.stringify(body);
  return new Request('https://api.lumascope.example.com/api/v1/activate', {
    ...init,
    method,
    headers: { 'Content-Type': 'application/json', ...(init?.headers ?? {}) },
    ...(method.toUpperCase() === 'GET' || method.toUpperCase() === 'HEAD'
      ? {}
      : { body: encodedBody }),
  });
}

function validBody(overrides?: Partial<ActivateRequest>): ActivateRequest {
  return {
    licenseKey: 'LICENSE-AAAA-BBBB-CCCC',
    machineId: 'machine-derived-id-v1',
    requestId: 'req_20260623_abcdef',
    timestamp: now.toISOString(),
    appVersion: '1.0.0',
    ...overrides,
  };
}

describe('activation API contracts', () => {
  it('defines the versioned v1 endpoints', () => {
    expect(ACTIVATION_ENDPOINTS).toEqual({
      activate: '/api/v1/activate',
      validate: '/api/v1/validate',
      deactivate: '/api/v1/deactivate',
    });
  });

  it('keeps stable non-leaking error codes and statuses', async () => {
    expect(ACTIVATION_ERROR_CODES).toEqual([
      'invalid_request',
      'license_not_found',
      'license_inactive',
      'activation_limit_reached',
      'activation_not_found',
      'machine_mismatch',
      'rate_limited',
      'replay_detected',
      'server_error',
    ]);

    for (const code of ACTIVATION_ERROR_CODES) {
      const envelope = createActivationError(code);
      expect(envelope.error.code).toBe(code);
      expect(envelope.error.message).not.toMatch(/sql|d1|stack|secret|license-/i);

      const response = activationErrorResponse(code);
      expect(response.status).toBe(activationStatusForError(code));
      const body = (await response.json()) as Record<string, unknown>;
      expect(JSON.stringify(body)).not.toMatch(/internal|trace|secret/i);
    }
  });

  it('models activation success without any embedded app secret field', () => {
    const response: ActivateResponse = {
      activationId: 'act_123',
      tokenType: 'lumascope-entitlement-v1',
      expiresAt: '2026-06-30T12:00:00.000Z',
      serverTime: now.toISOString(),
      refreshAfter: '2026-06-24T12:00:00.000Z',
      entitlementToken: {
        kid: 'test-key-1',
        algorithm: 'Ed25519',
        signature: 'signature',
        canonical: '{}',
        payload: {
          schemaVersion: 1,
          licenseKeyHash: 'sha256:abc123',
          activationId: 'act_123',
          machineId: 'machine-derived-id-v1',
          productId: 'prod_1',
          variantId: 'var_1',
          status: 'active',
          issuedAt: now.toISOString(),
          refreshAfter: '2026-06-24T12:00:00.000Z',
          expiresAt: '2026-06-30T12:00:00.000Z',
          kid: 'test-key-1',
        },
      },
    };

    expect(JSON.stringify(response)).not.toMatch(/appSecret|clientSecret|sharedSecret/i);
  });
});

describe('activation request validation', () => {
  it('accepts bounded activate request metadata', async () => {
    const result = await validateActivationJsonRequest<ActivateRequest>(
      request(validBody()),
      { now }
    );

    expect(result.ok).toBe(true);
    if (result.ok) {
      expect(result.value.licenseKey).toBe('LICENSE-AAAA-BBBB-CCCC');
      expect(result.value.machineId).toBe('machine-derived-id-v1');
      expect(result.value.requestId).toBe('req_20260623_abcdef');
      expect(result.value.appVersion).toBe('1.0.0');
    }
  });

  it('rejects non-POST, missing JSON content type, malformed JSON, and arrays', async () => {
    const cases = [
      request(validBody(), { method: 'GET' }),
      request(validBody(), { headers: { 'Content-Type': 'text/plain' } }),
      request('{not json'),
      request([]),
    ];

    for (const candidate of cases) {
      const result = await validateActivationJsonRequest(candidate, { now });
      expect(result.ok).toBe(false);
      if (!result.ok) {
        expect(result.error.error.code).toBe('invalid_request');
      }
    }
  });

  it('rejects missing, overlong, stale, or malformed fields before repository work', async () => {
    const stale = new Date(now.getTime() - 301_000).toISOString();
    const cases = [
      validBody({ licenseKey: '' }),
      validBody({ machineId: 'm'.repeat(257) }),
      validBody({ requestId: 'bad' }),
      validBody({ timestamp: stale }),
      validBody({ appVersion: 'v'.repeat(81) }),
    ];

    for (const body of cases) {
      const result = await validateActivationJsonRequest(request(body), { now });
      expect(result.ok).toBe(false);
    }
  });

  it('rejects oversized bodies using content length or actual body bytes', async () => {
    const withHeader = await validateActivationJsonRequest(
      request(validBody(), { headers: { 'Content-Length': '4097' } }),
      { now, maxBodyBytes: 4096 }
    );
    expect(withHeader.ok).toBe(false);
    if (!withHeader.ok) expect(withHeader.status).toBe(413);

    const withBody = await validateActivationJsonRequest(
      request(JSON.stringify({ value: 'x'.repeat(4097) })),
      { now, maxBodyBytes: 4096 }
    );
    expect(withBody.ok).toBe(false);
    if (!withBody.ok) expect(withBody.status).toBe(413);
  });
});
