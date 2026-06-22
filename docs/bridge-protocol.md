# Native/web bridge protocol

LumaScope protocol v1 uses JUCE native events. It never evaluates string-built JavaScript.

## UI ready

The loaded UI emits event `ui.ready` with exactly:

```json
{ "protocolVersion": 1 }
```

Extra fields, a non-integer version, and malformed payloads produce `bridge.error`.

## Host information

After a valid ready event, native emits `host.info`:

```json
{
  "protocolVersion": 1,
  "productName": "LumaScope",
  "companyName": "Signal Foundry Audio",
  "productVersion": "0.1.0",
  "hostMode": "Standalone",
  "uiSource": "embedded",
  "buildMarker": "0.1.0-Debug"
}
```

`hostMode` is `Standalone` or `VST3`; `uiSource` is `embedded` or `vite`. String fields are non-empty and bounded to 128 characters.

## Error envelope

Event `bridge.error` carries:

```json
{ "code": "protocol_mismatch", "message": "Human-readable bounded detail.", "protocolVersion": 1 }
```

Codes are bounded to 64 characters and messages to 256. Current stable codes include `malformed_payload`, `protocol_mismatch`, `runtime_error`, `development_server_unavailable`, `webview2_unavailable`, `embedded_resource_unavailable`, and `handshake_timeout`. Native owns errors that can occur before React loads; React owns protocol mismatch/runtime presentation after bundle startup.

## Versioning and extension

Protocol v1 payloads are closed schemas. Adding an optional or required field requires coordinated native/UI parser and fixture changes. A breaking semantic or shape change increments `protocolVersion`; unsupported versions fail explicitly instead of being guessed or downgraded. Rebuild native and UI from the same checkout after any protocol change.
