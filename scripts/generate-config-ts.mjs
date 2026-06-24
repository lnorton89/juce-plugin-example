#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";

const root = process.cwd();
const config = JSON.parse(fs.readFileSync(path.join(root, "project-config.json"), "utf8"));

const uiModule = `export const projectConfig = {
  productName: ${JSON.stringify(config.product.name)},
  companyName: ${JSON.stringify(config.product.companyName)},
  productVersion: ${JSON.stringify(config.version.string)},
  brandHeaderText: ${JSON.stringify(config.ui.brandHeaderText)},
  tagline: ${JSON.stringify(config.ui.tagline)},
  protocolVersion: ${JSON.stringify(config.bridge.protocolVersion)},
} as const;

export type ProjectConfig = typeof projectConfig;
`;

const workerModule = `export const projectConfig = {
  productName: ${JSON.stringify(config.product.name)},
  companyName: ${JSON.stringify(config.product.companyName)},
  tokenTypeV1: ${JSON.stringify(config.tokens.entitlementV1)},
  workerNames: ${JSON.stringify(config.worker.names, null, 2).replace(/\n/g, "\n  ")},
  d1DatabaseNames: ${JSON.stringify(config.worker.d1DatabaseNames, null, 2).replace(/\n/g, "\n  ")},
  routes: ${JSON.stringify(config.worker.routes, null, 2).replace(/\n/g, "\n  ")},
} as const;

export type EntitlementTokenType = typeof projectConfig.tokenTypeV1;
`;

const psModule = `$script:PRODUCT_NAME = '${config.product.name}'
$script:COMPANY_NAME = '${config.product.companyName}'
$script:PRODUCT_VERSION = '${config.version.string}'
$script:APPDATA_DIR_NAME = '${config.paths.appdataDirName}'
$script:SOURCE_PREFERENCE_FILENAME = '${config.paths.sourcePreferenceFilename}'
$script:ENTITLEMENT_FILENAME = '${config.paths.entitlementFilename}'
$script:PLUGIN_TARGET_NAME = '${config.build.pluginTargetName}'
$script:VST3_TARGET_NAME = '${config.build.vst3TargetName}'
$script:STANDALONE_TARGET_NAME = '${config.build.standaloneTargetName}'
$script:TEST_TARGET_NAME = '${config.build.testTargetName}'
$script:ARTEFACTS_DIR_NAME = '${config.build.artefactsDirName}'
`;

fs.mkdirSync(path.join(root, "ui/src/config"), { recursive: true });
fs.mkdirSync(path.join(root, "worker/src/config"), { recursive: true });
fs.writeFileSync(path.join(root, "ui/src/config/projectConfig.ts"), uiModule);
fs.writeFileSync(path.join(root, "worker/src/config/projectConfig.ts"), workerModule);
fs.writeFileSync(path.join(root, "scripts/config.ps1"), psModule);

console.log("Generated UI, Worker, and PowerShell project config modules");
