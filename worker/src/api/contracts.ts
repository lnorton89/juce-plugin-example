import type { ActivationErrorEnvelope } from './errors';
import type { EntitlementTokenType } from '../config/projectConfig';

export const ACTIVATION_API_VERSION = 'v1' as const;

export const ACTIVATION_ENDPOINTS = {
  activate: '/api/v1/activate',
  validate: '/api/v1/validate',
  deactivate: '/api/v1/deactivate',
} as const;

export type ActivationEndpointName = keyof typeof ACTIVATION_ENDPOINTS;

export interface ActivationRequestMetadata {
  licenseKey: string;
  machineId: string;
  requestId: string;
  timestamp: string;
  appVersion?: string;
}

export type ActivateRequest = ActivationRequestMetadata;
export type ValidateRequest = ActivationRequestMetadata;
export type DeactivateRequest = ActivationRequestMetadata;

export interface EntitlementClaims {
  schemaVersion: 1;
  licenseKeyHash: string;
  activationId: string;
  machineId: string;
  productId: string;
  variantId: string;
  status: 'active';
  issuedAt: string;
  refreshAfter: string;
  expiresAt: string;
  kid: string;
}

export interface SignedEntitlementToken {
  payload: EntitlementClaims;
  signature: string;
  kid: string;
  algorithm: 'Ed25519';
  canonical: string;
}

export interface ActivationSuccessResponse {
  activationId: string;
  entitlementToken: SignedEntitlementToken;
  tokenType: EntitlementTokenType;
  expiresAt: string;
  serverTime: string;
  refreshAfter: string;
}

export type ActivateResponse = ActivationSuccessResponse;
export type ValidateResponse = ActivationSuccessResponse;

export interface DeactivateResponse {
  status: 'deactivated';
  activationId: string;
  serverTime: string;
}

export type ActivationApiResponse =
  | ActivateResponse
  | ValidateResponse
  | DeactivateResponse
  | ActivationErrorEnvelope;

export type ActivationRequest =
  | ActivateRequest
  | ValidateRequest
  | DeactivateRequest;
