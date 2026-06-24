export const projectConfig = {
  productName: "LumaScope",
  companyName: "Signal Foundry Audio",
  productVersion: "0.1.0",
  brandHeaderText: "LumaScope",
  tagline: "Spectrum analyzer",
  protocolVersion: 1,
} as const;

export type ProjectConfig = typeof projectConfig;
