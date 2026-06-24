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
import { hashActivationIdentifiers, hashSensitiveValue, writeActivationAudit } from './audit';
import { checkActivationRateLimit } from './rateLimit';
import type { Env } from '../env';
import { Repository } from '../db/repository';
import type { Activation, License } from '../db/schema';
import { buildEntitlementClaims, signEntitlement } from '../signing/entitlement';
import { projectConfig } from '../config/projectConfig';

const TOKEN_TYPE = projectConfig.tokenTypeV1;
const REFRESH_AFTER_MS = 24 * 60 * 60 * 1000;
const EXPIRES_AFTER_MS = 7 * 24 * 60 * 60 * 1000;

export async function handleActivate(
  request: Request,
  env: Env,
  ctx: ExecutionContext
): Promise<Response> {
  const repository = new Repository(env.ACTIVATION_DB);
  const parsed = await validateActivationJsonRequest<ActivateRequest>(request);
  if (!parsed.ok) {
    ctx.waitUntil(audit(repository, request, 'activate', 'invalid_request'));
    return jsonError(parsed.status);
  }

  const preflight = await runActivationPreflight(repository, env, request, parsed.value, 'activate');
  if (preflight !== null) {
    ctx.waitUntil(audit(repository, request, 'activate', preflight.outcome, parsed.value));
    return preflight.response;
  }

  const license = await findActiveLicense(repository, parsed.value.licenseKey);
  if (license.response !== undefined) {
    ctx.waitUntil(audit(repository, request, 'activate', license.outcome, parsed.value));
    return license.response;
  }

  const now = new Date();
  const result = await repository.activateMachine(
    license.value.id,
    parsed.value.machineId,
    now.toISOString()
  );

  if (result.outcome === 'activation_limit_reached') {
    ctx.waitUntil(audit(repository, request, 'activate', 'activation_limit_reached', parsed.value));
    return activationErrorResponse('activation_limit_reached');
  }

  if (result.activation === null) {
    ctx.waitUntil(audit(repository, request, 'activate', 'server_error', parsed.value));
    return activationErrorResponse('server_error');
  }

  const activationId = activationIdFor(result.activation);
  ctx.waitUntil(audit(repository, request, 'activate', 'success', parsed.value, activationId));
  return jsonResponse(
    await buildActivationSuccessResponse(env, license.value, result.activation, now)
  );
}

export async function handleValidate(
  request: Request,
  env: Env,
  ctx: ExecutionContext
): Promise<Response> {
  const repository = new Repository(env.ACTIVATION_DB);
  const parsed = await validateActivationJsonRequest<ValidateRequest>(request);
  if (!parsed.ok) {
    ctx.waitUntil(audit(repository, request, 'validate', 'invalid_request'));
    return jsonError(parsed.status);
  }

  const preflight = await runActivationPreflight(repository, env, request, parsed.value, 'validate');
  if (preflight !== null) {
    ctx.waitUntil(audit(repository, request, 'validate', preflight.outcome, parsed.value));
    return preflight.response;
  }

  const license = await findActiveLicense(repository, parsed.value.licenseKey);
  if (license.response !== undefined) {
    ctx.waitUntil(audit(repository, request, 'validate', license.outcome, parsed.value));
    return license.response;
  }

  const now = new Date();
  const result = await repository.validateMachine(
    license.value.id,
    parsed.value.machineId,
    now.toISOString()
  );

  if (result.outcome === 'activation_not_found') {
    ctx.waitUntil(audit(repository, request, 'validate', 'activation_not_found', parsed.value));
    return activationErrorResponse('activation_not_found');
  }

  if (result.outcome === 'machine_mismatch') {
    ctx.waitUntil(audit(repository, request, 'validate', 'machine_mismatch', parsed.value));
    return activationErrorResponse('machine_mismatch');
  }

  if (result.activation === null) {
    ctx.waitUntil(audit(repository, request, 'validate', 'server_error', parsed.value));
    return activationErrorResponse('server_error');
  }

  const activationId = activationIdFor(result.activation);
  ctx.waitUntil(audit(repository, request, 'validate', 'success', parsed.value, activationId));
  return jsonResponse(
    await buildActivationSuccessResponse(env, license.value, result.activation, now)
  );
}

export async function handleDeactivate(
  request: Request,
  env: Env,
  ctx: ExecutionContext
): Promise<Response> {
  const repository = new Repository(env.ACTIVATION_DB);
  const parsed = await validateActivationJsonRequest<DeactivateRequest>(request);
  if (!parsed.ok) {
    ctx.waitUntil(audit(repository, request, 'deactivate', 'invalid_request'));
    return jsonError(parsed.status);
  }

  const preflight = await runActivationPreflight(repository, env, request, parsed.value, 'deactivate');
  if (preflight !== null) {
    ctx.waitUntil(audit(repository, request, 'deactivate', preflight.outcome, parsed.value));
    return preflight.response;
  }

  const license = await findActiveLicense(repository, parsed.value.licenseKey);
  if (license.response !== undefined) {
    ctx.waitUntil(audit(repository, request, 'deactivate', license.outcome, parsed.value));
    return license.response;
  }

  const now = new Date();
  const result = await repository.deactivateMachine(
    license.value.id,
    parsed.value.machineId,
    now.toISOString()
  );

  if (result.outcome === 'activation_not_found') {
    ctx.waitUntil(audit(repository, request, 'deactivate', 'activation_not_found', parsed.value));
    return activationErrorResponse('activation_not_found');
  }

  if (result.outcome === 'machine_mismatch') {
    ctx.waitUntil(audit(repository, request, 'deactivate', 'machine_mismatch', parsed.value));
    return activationErrorResponse('machine_mismatch');
  }

  if (result.activation === null) {
    ctx.waitUntil(audit(repository, request, 'deactivate', 'server_error', parsed.value));
    return activationErrorResponse('server_error');
  }

  const activationId = activationIdFor(result.activation);
  ctx.waitUntil(audit(repository, request, 'deactivate', 'deactivated', parsed.value, activationId));
  return jsonResponse({
    status: 'deactivated',
    activationId,
    serverTime: now.toISOString(),
  } satisfies DeactivateResponse);
}

async function findActiveLicense(
  repository: Repository,
  licenseKey: string
): Promise<
  | { value: License; response?: undefined; outcome?: undefined }
  | { value?: undefined; response: Response; outcome: 'license_not_found' | 'license_inactive' }
> {
  const license = await repository.findByLicenseKey(licenseKey);
  if (license === null) {
    return { response: activationErrorResponse('license_not_found'), outcome: 'license_not_found' };
  }

  if (license.status !== 'active') {
    return { response: activationErrorResponse('license_inactive'), outcome: 'license_inactive' };
  }

  return { value: license };
}

async function runActivationPreflight(
  repository: Repository,
  env: Env,
  request: Request,
  metadata: ActivateRequest | ValidateRequest | DeactivateRequest,
  action: 'activate' | 'validate' | 'deactivate'
): Promise<null | { response: Response; outcome: 'rate_limited' | 'replay_detected' }> {
  const route = new URL(request.url).pathname;
  const rateLimit = await checkActivationRateLimit(env, route, request, metadata);
  if (rateLimit.limited) {
    await markActivationRequest(repository, metadata, route, 'rate_limited');
    return { response: activationErrorResponse('rate_limited'), outcome: 'rate_limited' };
  }

  const existingRequest = await repository.findActivationRequest(metadata.requestId);
  if (existingRequest !== null) {
    await markActivationRequest(repository, metadata, route, 'replay_detected');
    return { response: activationErrorResponse('replay_detected'), outcome: 'replay_detected' };
  }

  await markActivationRequest(repository, metadata, route, 'accepted');
  return null;
}

async function markActivationRequest(
  repository: Repository,
  metadata: ActivateRequest | ValidateRequest | DeactivateRequest,
  route: string,
  status: 'accepted' | 'rate_limited' | 'replay_detected'
): Promise<void> {
  const hashes = await hashActivationIdentifiers(metadata);
  await repository.markActivationRequest({
    requestId: metadata.requestId,
    route,
    licenseKeyHash: hashes.licenseKeyHash,
    machineIdHash: hashes.machineIdHash,
    requestedAt: metadata.timestamp,
    status,
  });
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
  return hashSensitiveValue(licenseKey);
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

function audit(
  repository: Repository,
  request: Request,
  action: 'activate' | 'validate' | 'deactivate',
  outcome: Parameters<typeof writeActivationAudit>[1]['outcome'],
  metadata?: Partial<ActivateRequest | ValidateRequest | DeactivateRequest>,
  activationId?: string
): Promise<void> {
  return writeActivationAudit(repository, {
    action,
    outcome,
    route: new URL(request.url).pathname,
    method: request.method.toUpperCase(),
    request,
    metadata,
    activationId,
  });
}
