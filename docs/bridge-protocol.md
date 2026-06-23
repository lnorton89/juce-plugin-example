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

## Spectrum snapshot

After a valid ready handshake, native may emit `spectrum.snapshot` from the editor/message-thread polling path:

```json
{
  "protocolVersion": 1,
  "sequence": 42,
  "profile": "Musical",
  "sampleRate": 48000,
  "fftSize": 4096,
  "minFrequencyHz": 20,
  "maxFrequencyHz": 20000,
  "minDecibels": -96,
  "maxDecibels": 0,
  "bins": [
    { "frequencyHz": 20, "decibels": -96, "normalisedValue": 0 },
    { "frequencyHz": 1000, "decibels": -12, "normalisedValue": 0.875 }
  ]
}
```

`profile` is `Musical`, `Measurement`, or `Fast`. `bins` is non-empty and capped at 256 entries. Numeric fields must be finite; frequencies and sample rate are positive; `normalisedValue` is in `[0, 1]`. The audio callback never builds JSON or calls the WebView. Native converts the latest complete `SpectrumSnapshot` to this closed `juce::var` payload only on the editor/message side.

## Versioning and extension

Protocol v1 payloads are closed schemas. Adding an optional or required field requires coordinated native/UI parser and fixture changes. A breaking semantic or shape change increments `protocolVersion`; unsupported versions fail explicitly instead of being guessed or downgraded. Rebuild native and UI from the same checkout after any protocol change.
