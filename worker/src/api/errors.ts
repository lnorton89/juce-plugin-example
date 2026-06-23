export const ACTIVATION_ERROR_CODES = [
  'invalid_request',
  'license_not_found',
  'license_inactive',
  'activation_limit_reached',
  'activation_not_found',
  'machine_mismatch',
  'rate_limited',
  'replay_detected',
  'server_error',
] as const;

export type ActivationErrorCode = (typeof ACTIVATION_ERROR_CODES)[number];

export interface ActivationErrorEnvelope {
  error: {
    code: ActivationErrorCode;
    message: string;
  };
}

const STATUS_BY_CODE: Record<ActivationErrorCode, number> = {
  invalid_request: 400,
  license_not_found: 404,
  license_inactive: 403,
  activation_limit_reached: 409,
  activation_not_found: 404,
  machine_mismatch: 403,
  rate_limited: 429,
  replay_detected: 409,
  server_error: 500,
};

const MESSAGE_BY_CODE: Record<ActivationErrorCode, string> = {
  invalid_request: 'The request could not be processed.',
  license_not_found: 'The license could not be found.',
  license_inactive: 'The license is not active.',
  activation_limit_reached: 'The activation limit has been reached.',
  activation_not_found: 'The activation could not be found.',
  machine_mismatch: 'The activation does not match this machine.',
  rate_limited: 'Too many requests. Please try again later.',
  replay_detected: 'The request could not be processed.',
  server_error: 'The request could not be processed.',
};

export function createActivationError(
  code: ActivationErrorCode
): ActivationErrorEnvelope {
  return {
    error: {
      code,
      message: MESSAGE_BY_CODE[code],
    },
  };
}

export function activationStatusForError(code: ActivationErrorCode): number {
  return STATUS_BY_CODE[code];
}

export function activationErrorResponse(code: ActivationErrorCode): Response {
  return new Response(JSON.stringify(createActivationError(code)), {
    status: activationStatusForError(code),
    headers: { 'Content-Type': 'application/json' },
  });
}

