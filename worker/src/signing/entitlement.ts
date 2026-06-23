import type { EntitlementClaims, SignedEntitlementToken } from '../api/contracts';
import { canonicalJson, canonicalJsonBytes, type CanonicalJson } from './canonicalJson';

export interface EntitlementInput {
  licenseKeyHash: string;
  activationId: string;
  machineId: string;
  productId: string;
  variantId: string;
  issuedAt: string;
  refreshAfter: string;
  expiresAt: string;
  kid: string;
}

export interface PublicKeyRingEntry {
  kid: string;
  publicKey: string;
  algorithm: 'Ed25519';
}

export function buildEntitlementClaims(input: EntitlementInput): EntitlementClaims {
  return {
    schemaVersion: 1,
    licenseKeyHash: input.licenseKeyHash,
    activationId: input.activationId,
    machineId: input.machineId,
    productId: input.productId,
    variantId: input.variantId,
    status: 'active',
    issuedAt: input.issuedAt,
    refreshAfter: input.refreshAfter,
    expiresAt: input.expiresAt,
    kid: input.kid,
  };
}

export async function signEntitlement(
  claims: EntitlementClaims,
  privateKeyPkcs8Base64Url: string
): Promise<SignedEntitlementToken> {
  const key = await importEd25519PrivateKey(privateKeyPkcs8Base64Url);
  const canonical = canonicalJson(claims as unknown as CanonicalJson);
  const signature = await crypto.subtle.sign(
    { name: 'Ed25519' },
    key,
    new TextEncoder().encode(canonical)
  );

  return {
    payload: claims,
    signature: base64UrlEncode(new Uint8Array(signature)),
    kid: claims.kid,
    algorithm: 'Ed25519',
    canonical,
  };
}

export async function verifyEntitlementSignature(
  token: SignedEntitlementToken,
  publicKeySpkiBase64Url: string
): Promise<boolean> {
  const key = await importEd25519PublicKey(publicKeySpkiBase64Url);
  return crypto.subtle.verify(
    { name: 'Ed25519' },
    key,
    base64UrlDecode(token.signature),
    canonicalJsonBytes(token.payload as unknown as CanonicalJson)
  );
}

export async function importEd25519PrivateKey(
  privateKeyPkcs8Base64Url: string
): Promise<CryptoKey> {
  return crypto.subtle.importKey(
    'pkcs8',
    base64UrlDecode(privateKeyPkcs8Base64Url),
    { name: 'Ed25519' },
    false,
    ['sign']
  );
}

export async function importEd25519PublicKey(
  publicKeySpkiBase64Url: string
): Promise<CryptoKey> {
  return crypto.subtle.importKey(
    'spki',
    base64UrlDecode(publicKeySpkiBase64Url),
    { name: 'Ed25519' },
    false,
    ['verify']
  );
}

export function parsePublicKeyRing(rawJson: string): PublicKeyRingEntry[] {
  const parsed = JSON.parse(rawJson) as unknown;
  if (!Array.isArray(parsed)) {
    throw new Error('Signing public key ring must be an array');
  }

  return parsed.map((entry) => {
    if (!isPublicKeyRingEntry(entry)) {
      throw new Error('Invalid signing public key ring entry');
    }
    return entry;
  });
}

export function base64UrlEncode(bytes: Uint8Array): string {
  let binary = '';
  for (const byte of bytes) {
    binary += String.fromCharCode(byte);
  }
  return btoa(binary).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/g, '');
}

export function base64UrlDecode(value: string): Uint8Array {
  const padded = value.replace(/-/g, '+').replace(/_/g, '/').padEnd(
    Math.ceil(value.length / 4) * 4,
    '='
  );
  const binary = atob(padded);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i += 1) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes;
}

function isPublicKeyRingEntry(value: unknown): value is PublicKeyRingEntry {
  if (value === null || typeof value !== 'object') {
    return false;
  }
  const entry = value as Record<string, unknown>;
  return (
    typeof entry.kid === 'string' &&
    typeof entry.publicKey === 'string' &&
    entry.algorithm === 'Ed25519'
  );
}

