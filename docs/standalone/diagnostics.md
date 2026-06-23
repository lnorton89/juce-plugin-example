# Standalone Diagnostics

## Inline Diagnostics (PluginEditor.cpp)

The plugin editor registers source event handlers and logs diagnostics at startup and during source transitions.

### Source Events
- `source.list`: On request, enumerates available input devices and render endpoints.
- `source.state`: Emitted on every state transition (starting, active, silent, stopped, error).
- `source.error`: Emitted when a source operation fails, with `code` and `message`.

### Bridge Protocol Messages
- `source.select(mode, sourceId)`: Request to start capture for a given source.
- `source.stop()`: Request to stop the current capture.

### Error Codes

| Code | Meaning | Recovery |
|------|---------|----------|
| `invalid_selection` | Empty or missing source ID | Select a valid source from the dropdown |
| `device_init_failed` | JUCE AudioDeviceManager initialization failed | Check device availability |
| `endpoint_start_failed` | WASAPI loopback endpoint start failed | Check endpoint validity |
| `endpoint_lost` | WASAPI endpoint removed or failed during capture | Select a different endpoint |
| `endpoint_changed` | Default device changed; endpoint affected | Reselect desired endpoint |
| `unknown_mode` | Invalid source mode value | Use UI to select mode |
| No source state | Source strip initial state | Choose a source |

## Native Diagnostics (PluginProcessor)

When `JucePlugin_Build_Standalone` is defined:
- `standaloneSourceController` is created to manage input capture
- The controller emits state changes visible in the bridge `source.state` event

## Frontend Diagnostics

In the standalone source strip:
- State is displayed as a status message under the source selection controls
- Error states show the error code and a human-readable message
- The strip appears only when `hostMode === 'Standalone'`
- VST3 builds unconditionally hide the source strip
