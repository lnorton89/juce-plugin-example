#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const root = process.cwd();
const configPath = path.join(root, "project-config.json");
const schemaPath = path.join(root, "project-config.schema.json");

function fail(message) {
  console.error(`project-config validation failed: ${message}`);
  process.exit(1);
}

function readJson(filePath) {
  try {
    return JSON.parse(fs.readFileSync(filePath, "utf8"));
  } catch (error) {
    fail(`${path.relative(root, filePath)} is missing or invalid JSON: ${error.message}`);
  }
}

function isObject(value) {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function validateValue(value, schema, dottedPath) {
  const where = dottedPath || "<root>";

  if (schema.type === "object") {
    if (!isObject(value)) {
      fail(`${where} must be an object`);
    }

    for (const key of schema.required ?? []) {
      if (!Object.prototype.hasOwnProperty.call(value, key)) {
        fail(`${where}.${key}`.replace(/^<root>\./, "") + " is required");
      }
    }

    if (schema.additionalProperties === false) {
      for (const key of Object.keys(value)) {
        if (!schema.properties || !Object.prototype.hasOwnProperty.call(schema.properties, key)) {
          fail(`${where}.${key}`.replace(/^<root>\./, "") + " is not allowed by project-config.schema.json");
        }
      }
    }

    for (const [key, childSchema] of Object.entries(schema.properties ?? {})) {
      if (Object.prototype.hasOwnProperty.call(value, key)) {
        validateValue(value[key], childSchema, dottedPath ? `${dottedPath}.${key}` : key);
      }
    }
    return;
  }

  if (!Array.isArray(schema.filesAffected)) {
    fail(`schema property ${where} must declare filesAffected`);
  }

  if (schema.type === "string") {
    if (typeof value !== "string") {
      fail(`${where} must be a string`);
    }
    if (schema.minLength !== undefined && value.length < schema.minLength) {
      fail(`${where} must be at least ${schema.minLength} character(s)`);
    }
    if (schema.maxLength !== undefined && value.length > schema.maxLength) {
      fail(`${where} must be at most ${schema.maxLength} character(s)`);
    }
    if (schema.pattern && !new RegExp(schema.pattern).test(value)) {
      fail(`${where} must match ${schema.pattern}`);
    }
    return;
  }

  if (schema.type === "integer") {
    if (!Number.isInteger(value)) {
      fail(`${where} must be an integer`);
    }
    if (schema.minimum !== undefined && value < schema.minimum) {
      fail(`${where} must be >= ${schema.minimum}`);
    }
    return;
  }

  fail(`schema property ${where} has unsupported type '${schema.type}'`);
}

const config = readJson(configPath);
const schema = readJson(schemaPath);

if (schema.$version !== "1.0.0") {
  fail(`project-config.schema.json $version must be 1.0.0; found '${schema.$version ?? "<missing>"}'`);
}

validateValue(config, schema, "");

if (config.version.string !== `${config.version.major}.${config.version.minor}.${config.version.patch}`) {
  fail("version.string must equal version.major.version.minor.version.patch");
}

console.log(`project-config OK: ${config.product.name} ${config.version.string}`);
