import type { ActivationRequestMetadata } from './contracts';
import type { ActivationErrorCode } from './errors';
import { Repository } from '../db/repository';
import { base64UrlEncode } from '../signing/entitlement';

export type ActivationAuditAction = 'activate' | 'validate' | 'deactivate';
export type ActivationAuditOutcome = ActivationErrorCode | 'success' | 'deactivated';

export interface ActivationAuditInput {
  action: ActivationAuditAction;
  outcome: ActivationAuditOutcome;
  route: string;
  method: string;
  request: Request;
  metadata?: Partial<ActivationRequestMetadata>;
  activationId?: string;
}

export interface ActivationIdentifierHashes {
  licenseKeyHash?: string;
  machineIdHash?: string;
}

export async function writeActivationAudit(
  repository: Repository,
  input: ActivationAuditInput
): Promise<void> {
  const hashes = await hashActivationIdentifiers(input.metadata);
  const details = {
    action: input.action,
    outcome: input.outcome,
    route: input.route,
    method: input.method,
    licenseKeyHash: hashes.licenseKeyHash ?? null,
    machineIdHash: hashes.machineIdHash ?? null,
    activationId: input.activationId ?? null,
    requestId: input.metadata?.requestId ?? null,
    appVersion: input.metadata?.appVersion ?? null,
    client: coarseClientInfo(input.request),
  };

  await repository.writeAudit(
    `activation_${input.action}`,
    'activation_api',
    'activation',
    input.activationId,
    JSON.stringify(details),
    undefined,
    input.metadata?.requestId
  );
}

export async function hashActivationIdentifiers(
  metadata?: Partial<ActivationRequestMetadata>
): Promise<ActivationIdentifierHashes> {
  return {
    ...(metadata?.licenseKey ? { licenseKeyHash: await hashSensitiveValue(metadata.licenseKey) } : {}),
    ...(metadata?.machineId ? { machineIdHash: await hashSensitiveValue(metadata.machineId) } : {}),
  };
}

export async function hashSensitiveValue(value: string): Promise<string> {
  const digest = await crypto.subtle.digest('SHA-256', new TextEncoder().encode(value));
  return `sha256:${base64UrlEncode(new Uint8Array(digest))}`;
}

function coarseClientInfo(request: Request): Record<string, string | null> {
  return {
    userAgentFamily: coarseUserAgent(request.headers.get('User-Agent')),
    colo: request.headers.get('CF-Ray') !== null ? 'cloudflare' : null,
  };
}

function coarseUserAgent(userAgent: string | null): string | null {
  if (userAgent === null || userAgent.length === 0) {
    return null;
  }

  if (/lumascope/i.test(userAgent)) return 'lumascope';
  if (/curl/i.test(userAgent)) return 'curl';
  if (/mozilla/i.test(userAgent)) return 'browser';
  return 'other';
}

