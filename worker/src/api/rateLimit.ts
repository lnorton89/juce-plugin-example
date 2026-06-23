import type { ActivationRequestMetadata } from './contracts';
import { hashActivationIdentifiers } from './audit';
import type { Env } from '../env';

export interface ActivationRateLimitResult {
  limited: boolean;
  keys: string[];
}

export async function checkActivationRateLimit(
  env: Env,
  route: string,
  request: Request,
  metadata: ActivationRequestMetadata
): Promise<ActivationRateLimitResult> {
  const binding = env.ACTIVATION_RATE_LIMIT;
  const keys = await buildActivationRateLimitKeys(route, request, metadata);

  if (binding === undefined) {
    return { limited: false, keys };
  }

  for (const key of keys) {
    const result = await binding.limit({ key });
    if (!result.success) {
      return { limited: true, keys };
    }
  }

  return { limited: false, keys };
}

export async function buildActivationRateLimitKeys(
  route: string,
  request: Request,
  metadata: ActivationRequestMetadata
): Promise<string[]> {
  const hashes = await hashActivationIdentifiers(metadata);
  const keys: string[] = [];
  const ip = request.headers.get('CF-Connecting-IP');

  if (ip !== null && ip.length > 0) {
    keys.push(`${route}:ip:${ip}`);
  }

  if (hashes.licenseKeyHash !== undefined) {
    keys.push(`${route}:license:${hashes.licenseKeyHash}`);
  }

  if (hashes.machineIdHash !== undefined) {
    keys.push(`${route}:machine:${hashes.machineIdHash}`);
  }

  return keys;
}

