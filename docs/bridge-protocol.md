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

## Source list (standalone only)

After a `host.info` with `hostMode: "Standalone"`, native may emit `source.list` containing available capture endpoints.

```json
{
  "protocolVersion": 1,
  "event": "source.list",
  "inputDevices": [
    { "id": "juce-input-mic-1", "displayName": "Microphone (Realtek Audio)", "mode": "InputDevice" }
  ],
  "systemOutputs": [
    { "id": "wasapi-speaker-1", "displayName": "Speakers (Realtek Audio)", "mode": "SystemOutput" }
  ]
}
```

- `inputDevices` and `systemOutputs` are separate arrays (D-04). They are never combined into one ambiguous list.
- `mode` is `"InputDevice"` or `"SystemOutput"` — a closed enum defined by `SourceMode` on the native side.
- Source IDs and display names are non-empty strings bounded to 256 characters.
- VST3 builds never emit `source.list`; the UI must branch on `host.info.hostMode`.

## Source state (standalone only)

Native emits `source.state` whenever the standalone capture lifecycle changes.

```json
{
  "protocolVersion": 1,
  "event": "source.state",
  "mode": "InputDevice",
  "state": "active",
  "selectedSourceId": "juce-input-mic-1",
  "selectedSourceName": "Microphone (Realtek Audio)",
  "code": "",
  "message": ""
}
```

### Source states

| State | Meaning |
|-------|---------|
| `stopped` | No source selected or capture is idle. `selectedSourceId` is empty. |
| `starting` | A source was selected and capture is being set up. |
| `active` | Capture is running and processing audio normally. |
| `silent` | Source is valid but no signal is detected (D-08). Not an error. |
| `error` | Source failed or was lost. `code` and `message` provide details. User must select a new source (D-05, D-06). |

### Error codes

| Code | Meaning |
|------|---------|
| `source_lost` | Selected device was disconnected or became invalid. |
| `device_init_failed` | Could not open or initialize the selected device. |
| `invalid_selection` | Selection payload was missing required fields. |
| `not_implemented` | The requested source mode is not yet available. |

- `code` is bounded to 64 characters, `message` to 256.
- `selectedSourceId` and `selectedSourceName` are cleared when state is `stopped`.
- No auto-fallback to a different source on failure (D-06). Silent sources stay active with status `silent` (D-08).

## UI request events

The UI may emit the following events to control standalone capture. These are not routed from any capture callback (real-time safe by design).

### `source.select`

```json
{ "protocolVersion": 1, "mode": "InputDevice", "sourceId": "juce-input-mic-1" }
```

Requests native to select the specified source. Native validates mode and source ID against the current enumeration before acting.

### `source.stop`

```json
{ "protocolVersion": 1 }
```

Requests native to stop the currently active source and return to `stopped` state.

## Versioning and extension

Protocol v1 payloads are closed schemas. Adding an optional or required field requires coordinated native/UI parser and fixture changes. A breaking semantic or shape change increments `protocolVersion`; unsupported versions fail explicitly instead of being guessed or downgraded. Rebuild native and UI from the same checkout after any protocol change.
