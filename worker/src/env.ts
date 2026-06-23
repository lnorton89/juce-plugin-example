export interface Env {
  ACTIVATION_DB: D1Database;
  LEMON_WEBHOOK_SECRET: string;
  LEMON_STORE_ID: string;
  LEMON_PRODUCT_ID: string;
  LEMON_VARIANT_ID: string;
  SIGNING_PRIVATE_KEY: string;
  SIGNING_KEY_ID: string;
  SIGNING_PUBLIC_KEYS: string;
  ENVIRONMENT: string;
}
