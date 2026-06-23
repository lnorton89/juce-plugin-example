import { describe, it, expect } from 'vitest';
import { canonicalJson, canonicalJsonBytes } from '../../worker/src/signing/canonicalJson';
import {
  base64UrlEncode,
  buildEntitlementClaims,
  parsePublicKeyRing,
  signEntitlement,
  verifyEntitlementSignature,
} from '../../worker/src/signing/entitlement';
import fixture from './fixtures/entitlement-v1.json';

describe('canonicalJson', () => {
  it('sorts object keys recursively and preserves array order', () => {
    const canonical = canonicalJson({
      z: 1,
      a: { c: true, b: 'text' },
      list: [{ y: null, x: 2 }],
    });

    expect(canonical).toBe('{"a":{"b":"text","c":true},"list":[{"x":2,"y":null}],"z":1}');
    expect(new TextDecoder().decode(canonicalJsonBytes({ b: 2, a: 1 }))).toBe('{"a":1,"b":2}');
  });
});

describe('entitlement signing', () => {
  it('builds minimal claims without raw license key or customer PII', () => {
    const claims = buildEntitlementClaims(fixture.input);

    expect(claims).toEqual(fixture.payload);
    expect(JSON.stringify(claims)).not.toMatch(/LICENSE-|customer|email|Jane/i);
  });

  it('signs canonical claims and verifies the fixture key ring', async () => {
    const claims = buildEntitlementClaims(fixture.input);
    const token = await signEntitlement(claims, fixture.privateKeyPkcs8);

    expect(token.canonical).toBe(fixture.canonical);
    expect(token.signature).toBe(fixture.signature);
    expect(token.kid).toBe('test-ed25519-2026-06');
    expect(await verifyEntitlementSignature(token, fixture.publicKeySpki)).toBe(true);

    const ring = parsePublicKeyRing(JSON.stringify(fixture.publicKeyRing));
    expect(ring).toEqual([
      {
        kid: 'test-ed25519-2026-06',
        publicKey: fixture.publicKeySpki,
        algorithm: 'Ed25519',
      },
    ]);
  });

  it('uses unpadded base64url encoding for binary material', () => {
    expect(base64UrlEncode(new Uint8Array([251, 255, 238]))).toBe('-__u');
  });
});

