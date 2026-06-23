export type LicenseStatus = 'active' | 'expired' | 'refunded' | 'disabled' | 'pending';
export type IdempotencyStatus = 'completed' | 'failed' | 'skipped';

export interface License {
  id: number;
  lemon_order_id: string;
  license_key: string;
  product_id: string;
  variant_id: string;
  store_id: string;
  status: LicenseStatus;
  customer_name: string | null;
  customer_email: string | null;
  activated_count: number;
  created_at: string;
  updated_at: string;
}

export interface Activation {
  id: number;
  license_id: number;
  machine_identifier: string;
  activated_at: string;
  last_validated_at: string;
  deactivated_at: string | null;
  is_active: boolean;
}

export type ActivationPolicyOutcome =
  | 'activated'
  | 'refreshed'
  | 'activation_limit_reached'
  | 'activation_not_found'
  | 'machine_mismatch'
  | 'deactivated';

export interface ActivationPolicyResult {
  outcome: ActivationPolicyOutcome;
  activation: Activation | null;
}

export interface WebhookIdempotency {
  id: number;
  event_id: string;
  event_type: string;
  event_created_at: string;
  processed_at: string;
  status: IdempotencyStatus;
  result_summary: string | null;
}

export interface AuditLogEntry {
  id: number;
  action: string;
  actor: string;
  resource_type: string;
  resource_id: string | null;
  details: string | null;
  ip_address: string | null;
  timestamp: string;
  request_id: string | null;
}

export interface RepositoryConfig {
  allProducts: string[];
  allStores: string[];
  allVariants: string[];
}

export type D1ResultRow = Record<string, unknown>;
