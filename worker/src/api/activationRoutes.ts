import type {
  ActivateRequest,
  ActivateResponse,
  DeactivateRequest,
  DeactivateResponse,
  EntitlementClaims,
  ValidateRequest,
  ValidateResponse,
} from './contracts';
import { activationErrorResponse } from './errors';
import { validateActivationJsonRequest } from './requestValidation';
import type { Env } from '../env';
import { Repository } from '../db/repository';
import type { Activation, License } from '../db/schema';
import { base64UrlEncode, buildEntitlementClaims, signEntitlement } from '../signing/entitlement';

const TOKEN_TYPE = 'lumascope-entitlement-v1' as const;
const REFRESH_AFTER_MS = 24 * 60 * 60 * 1000;
const EXPIRES_AFTER_MS = 7 * 24 * 60 * 60 * 1000;

export async function handleActivate(request: Request, env: Env): Promise<Response> {
  const parsed = await validateActivationJsonRequest<ActivateRequest>(request);
  if (!parsed.ok) {
    return jsonError(parsed.status);
  }

  const repository = new Repository(env.ACTIVATION_DB);
  const license = await findActiveLicense(repository, parsed.value.licenseKey);
  if (license.response !== undefined) {
    return license.response;
  }

  const now = new Date();
  const result = await repository.activateMachine(
    license.value.id,
    parsed.value.machineId,
    now.toISOString()
  );

  if (result.outcome === 'activation_limit_reached') {
    return activationErrorResponse('activation_limit_reached');
  }

  if (result.activation === null) {
    return activationErrorResponse('server_error');
  }

  return jsonResponse(
    await buildActivationSuccessResponse(env, license.value, result.activation, now)
  );
}

export async function handleValidate(request: Request, env: Env): Promise<Response> {
  const parsed = await validateActivationJsonRequest<ValidateRequest>(request);
  if (!parsed.ok) {
    return jsonError(parsed.status);
  }

  const repository = new Repository(env.ACTIVATION_DB);
  const license = await findActiveLicense(repository, parsed.value.licenseKey);
  if (license.response !== undefined) {
    return license.response;
  }

  const now = new Date();
  const result = await repository.validateMachine(
    license.value.id,
    parsed.value.machineId,
    now.toISOString()
  );

  if (result.outcome === 'activation_not_found') {
    return activationErrorResponse('activation_not_found');
  }

  if (result.outcome === 'machine_mismatch') {
    return activationErrorResponse('machine_mismatch');
  }

  if (result.activation === null) {
    return activationErrorResponse('server_error');
  }

  return jsonResponse(
    await buildActivationSuccessResponse(env, license.value, result.activation, now)
  );
}

export async function handleDeactivate(request: Request, env: Env): Promise<Response> {
  const parsed = await validateActivationJsonRequest<DeactivateRequest>(request);
  if (!parsed.ok) {
    return jsonError(parsed.status);
  }

  const repository = new Repository(env.ACTIVATION_DB);
  const license = await findActiveLicense(repository, parsed.value.licenseKey);
  if (license.response !== undefined) {
    return license.response;
  }

  const now = new Date();
  const result = await repository.deactivateMachine(
    license.value.id,
    parsed.value.machineId,
    now.toISOString()
  );

  if (result.outcome === 'activation_not_found') {
    return activationErrorResponse('activation_not_found');
  }

  if (result.outcome === 'machine_mismatch') {
    return activationErrorResponse('machine_mismatch');
  }

  if (result.activation === null) {
    return activationErrorResponse('server_error');
  }

  return jsonResponse({
    status: 'deactivated',
    activationId: activationIdFor(result.activation),
    serverTime: now.toISOString(),
  } satisfies DeactivateResponse);
}

async function findActiveLicense(
  repository: Repository,
  licenseKey: string
): Promise<{ value: License; response?: undefined } | { value?: undefined; response: Response }> {
  const license = await repository.findByLicenseKey(licenseKey);
  if (license === null) {
    return { response: activationErrorResponse('license_not_found') };
  }

  if (license.status !== 'active') {
    return { response: activationErrorResponse('license_inactive') };
  }

  return { value: license };
}

async function buildActivationSuccessResponse(
  env: Env,
  license: License,
  activation: Activation,
  now: Date
): Promise<ActivateResponse | ValidateResponse> {
  const issuedAt = now.toISOString();
  const refreshAfter = new Date(now.getTime() + REFRESH_AFTER_MS).toISOString();
  const expiresAt = new Date(now.getTime() + EXPIRES_AFTER_MS).toISOString();
  const kid = env.SIGNING_KEY_ID || 'default';

  if (!env.SIGNING_PRIVATE_KEY) {
    throw new Error('SIGNING_PRIVATE_KEY is required to issue entitlements');
  }

  const claims: EntitlementClaims = buildEntitlementClaims({
    licenseKeyHash: await hashLicenseKey(license.license_key),
    activationId: activationIdFor(activation),
    machineId: activation.machine_identifier,
    productId: license.product_id,
    variantId: license.variant_id,
    issuedAt,
    refreshAfter,
    expiresAt,
    kid,
  });

  return {
    activationId: claims.activationId,
    entitlementToken: await signEntitlement(claims, env.SIGNING_PRIVATE_KEY),
    tokenType: TOKEN_TYPE,
    expiresAt,
    serverTime: issuedAt,
    refreshAfter,
  };
}

function activationIdFor(activation: Activation): string {
  return `act_${activation.id}`;
}

async function hashLicenseKey(licenseKey: string): Promise<string> {
  const digest = await crypto.subtle.digest('SHA-256', new TextEncoder().encode(licenseKey));
  return `sha256:${base64UrlEncode(new Uint8Array(digest))}`;
}

function jsonResponse(body: unknown): Response {
  return new Response(JSON.stringify(body), {
    status: 200,
    headers: { 'Content-Type': 'application/json' },
  });
}

function jsonError(status: number): Response {
  return new Response(JSON.stringify({ error: { code: 'invalid_request', message: 'The request could not be processed.' } }), {
    status,
    headers: { 'Content-Type': 'application/json' },
  });
}

