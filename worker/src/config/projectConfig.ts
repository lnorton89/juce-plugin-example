export const projectConfig = {
  productName: "LumaScope",
  companyName: "Signal Foundry Audio",
  tokenTypeV1: "lumascope-entitlement-v1",
  workerNames: {
    "production": "lumascope-activation",
    "preview": "lumascope-activation-preview",
    "local": "lumascope-activation-local"
  },
  d1DatabaseNames: {
    "production": "lumascope-activation-db",
    "preview": "lumascope-activation-db-preview",
    "local": "lumascope-activation-db-local"
  },
  routes: {
    "production": "api.lumascope.example.com/*",
    "preview": "preview-api.lumascope.example.com/*"
  },
} as const;

export type EntitlementTokenType = typeof projectConfig.tokenTypeV1;
