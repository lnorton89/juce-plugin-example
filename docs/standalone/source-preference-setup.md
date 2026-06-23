# Source Preference Persistence

## File Location

```
%APPDATA%\LumaScope\source-preference.json
```

## Schema

```json
{
  "schemaVersion": 1,
  "mode": "InputDevice",
  "id": "juce-input-Microphone (Realtek Audio)",
  "displayName": "Microphone (Realtek Audio)"
}
```

| Field | Type | Description |
|-------|------|-------------|
| `schemaVersion` | int | Currently `1`; used for forward compatibility |
| `mode` | string | `"InputDevice"` or `"SystemOutput"` |
| `id` | string | Device identifier for the source adapter |
| `displayName` | string | Human-readable device name (max 256 chars) |

## Persistence Rules

### When a preference is saved
- **Only** after a successful `selectSource` that returns `active`, `starting`, or `silent` state.
- Not persisted for failed selections or error states.

### When a preference is restored
1. On `StandaloneSourceController` construction (standalone startup).
2. If `tryRestore()` returns a valid saved selection:
   - The saved source is looked up in the current device enumeration.
   - If found, `selectSource` is called with the saved selection automatically.
   - If **not** found, the application starts in the `stopped` state (no auto-fallback to a different source).
3. If `tryRestore()` returns empty (no saved file, corrupt data, unknown schema):
   - Application starts in the `stopped` state.
   - The user must choose a source manually.

### When a preference is cleared
- `clear()` deletes the preference file.
- Reserved for explicit deactivation or factory reset flows (currently unused from UI).

## Testing

See `SourcePreferenceStoreTests.cpp` for test coverage including:

| Test | Description |
|------|-------------|
| CAP-05 Save and restore | Full save/restore round-trip |
| D-14 Missing file | Returns empty when file is absent |
| SystemOutput save/restore | Verifies both mode values |
| CAP-05 Clear store | Empty after clear |
| Corrupt data | Invalid JSON returns empty |
| Overwrite on repeated save | Last saved value wins |
