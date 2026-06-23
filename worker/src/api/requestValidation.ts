import type { ActivationRequestMetadata } from './contracts';
import { createActivationError, type ActivationErrorEnvelope } from './errors';

export interface ActivationValidationOptions {
  now?: Date;
  maxBodyBytes?: number;
  maxTimestampSkewMs?: number;
}

export interface ActivationValidationSuccess<T extends ActivationRequestMetadata> {
  ok: true;
  value: T;
}

export interface ActivationValidationFailure {
  ok: false;
  status: number;
  error: ActivationErrorEnvelope;
}

export type ActivationValidationResult<T extends ActivationRequestMetadata> =
  | ActivationValidationSuccess<T>
  | ActivationValidationFailure;

const DEFAULT_MAX_BODY_BYTES = 4096;
const DEFAULT_MAX_TIMESTAMP_SKEW_MS = 300_000;
const LICENSE_KEY_MAX_LENGTH = 256;
const MACHINE_ID_MAX_LENGTH = 256;
const REQUEST_ID_MAX_LENGTH = 80;
const APP_VERSION_MAX_LENGTH = 80;
const REQUEST_ID_PATTERN = /^[A-Za-z0-9][A-Za-z0-9._:-]{7,79}$/;

type StringField = 'licenseKey' | 'machineId' | 'requestId' | 'timestamp' | 'appVersion';

export async function validateActivationJsonRequest<T extends ActivationRequestMetadata>(
  request: Request,
  options: ActivationValidationOptions = {}
): Promise<ActivationValidationResult<T>> {
  const method = request.method.toUpperCase();
  if (method !== 'POST') {
    return invalid();
  }

  const contentType = request.headers.get('Content-Type') || '';
  if (!contentType.toLowerCase().includes('application/json')) {
    return invalid();
  }

  const maxBodyBytes = options.maxBodyBytes ?? DEFAULT_MAX_BODY_BYTES;
  const contentLength = request.headers.get('Content-Length');
  if (contentLength !== null && Number.parseInt(contentLength, 10) > maxBodyBytes) {
    return invalid(413);
  }

  let body: string;
  try {
    body = await request.text();
  } catch {
    return invalid();
  }

  if (new TextEncoder().encode(body).byteLength > maxBodyBytes) {
    return invalid(413);
  }

  let parsed: unknown;
  try {
    parsed = JSON.parse(body);
  } catch {
    return invalid();
  }

  if (!isPlainObject(parsed)) {
    return invalid();
  }

  const required: StringField[] = ['licenseKey', 'machineId', 'requestId', 'timestamp'];
  for (const field of required) {
    if (!isNonEmptyBoundedString(parsed[field], maxLengthFor(field))) {
      return invalid();
    }
  }

  if (
    parsed.appVersion !== undefined &&
    !isNonEmptyBoundedString(parsed.appVersion, APP_VERSION_MAX_LENGTH)
  ) {
    return invalid();
  }

  const requestId = parsed.requestId as string;
  if (!REQUEST_ID_PATTERN.test(requestId)) {
    return invalid();
  }

  if (!isTimestampAcceptable(parsed.timestamp as string, options)) {
    return invalid();
  }

  return {
    ok: true,
    value: {
      licenseKey: parsed.licenseKey as string,
      machineId: parsed.machineId as string,
      requestId,
      timestamp: parsed.timestamp as string,
      ...(parsed.appVersion !== undefined ? { appVersion: parsed.appVersion as string } : {}),
    } as T,
  };
}

function invalid(status: number = 400): ActivationValidationFailure {
  return {
    ok: false,
    status,
    error: createActivationError('invalid_request'),
  };
}

function isPlainObject(value: unknown): value is Record<string, unknown> {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function isNonEmptyBoundedString(value: unknown, maxLength: number): value is string {
  return typeof value === 'string' && value.length > 0 && value.length <= maxLength;
}

function maxLengthFor(field: StringField): number {
  switch (field) {
    case 'licenseKey':
      return LICENSE_KEY_MAX_LENGTH;
    case 'machineId':
      return MACHINE_ID_MAX_LENGTH;
    case 'requestId':
      return REQUEST_ID_MAX_LENGTH;
    case 'timestamp':
      return 40;
    case 'appVersion':
      return APP_VERSION_MAX_LENGTH;
  }
}

function isTimestampAcceptable(
  timestamp: string,
  options: ActivationValidationOptions
): boolean {
  const parsed = Date.parse(timestamp);
  if (Number.isNaN(parsed)) {
    return false;
  }

  const now = options.now?.getTime() ?? Date.now();
  const maxSkew = options.maxTimestampSkewMs ?? DEFAULT_MAX_TIMESTAMP_SKEW_MS;
  return Math.abs(now - parsed) <= maxSkew;
}

