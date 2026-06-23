#pragma once

#include <juce_core/juce_core.h>

#include <cstdint>
#include <vector>

namespace lumascope
{

// D-01: Two distinct source modes, never one ambiguous list.
enum class SourceMode : std::uint8_t
{
    inputDevice,
    systemOutput
};

// D-05: Source lifecycle states.
enum class SourceState : std::uint8_t
{
    stopped,   // No source selected or active capture
    starting,  // Source is being opened/initialized
    active,    // Source is capturing normally
    silent,    // Source is valid but no signal detected (D-08)
    error      // Source has failed and needs user action (D-05)
};

// A single source endpoint.
// IDs and display names are bounded and non-empty for valid entries.
struct SourceDescriptor
{
    juce::String id;          // Stable bounded identifier, max 256 chars
    juce::String displayName; // Human-readable name, max 256 chars
    SourceMode mode = SourceMode::inputDevice;
};

// Bounded lists for each mode — never combined (D-04).
struct SourceList
{
    std::vector<SourceDescriptor> inputDevices;
    std::vector<SourceDescriptor> systemOutputs;
};

// A user's selected source choice.
struct SourceSelection
{
    SourceMode mode = SourceMode::inputDevice;
    juce::String id;
    juce::String displayName;
};

// Current source lifecycle state with bounded error fields.
// This is a closed payload ready for protocol-v1 emission.
struct SourceStateSnapshot
{
    SourceMode mode = SourceMode::inputDevice;
    SourceState state = SourceState::stopped;
    juce::String selectedSourceId;   // ID of currently selected source, empty if stopped
    juce::String selectedSourceName; // Display name, bounded to 256
    juce::String code;               // Machine-readable error code (non-empty only when state == error), bounded to 64
    juce::String message;            // Human-readable error detail (non-empty only when state == error), bounded to 256
};

// String conversions for protocol-v1 emission.
inline const char* toString (SourceMode mode) noexcept
{
    switch (mode)
    {
        case SourceMode::inputDevice: return "InputDevice";
        case SourceMode::systemOutput: return "SystemOutput";
        default: return "InputDevice";
    }
}

inline const char* toString (SourceState state) noexcept
{
    switch (state)
    {
        case SourceState::stopped:  return "stopped";
        case SourceState::starting: return "starting";
        case SourceState::active:   return "active";
        case SourceState::silent:   return "silent";
        case SourceState::error:    return "error";
        default: return "stopped";
    }
}

// ============================================================================
// WASAPI render endpoint enumeration (Windows only).
// Returns all system-output render endpoints for the SystemOutput mode.
// ============================================================================

// Enumerate all eRender endpoints via WASAPI COM.
// Returns an empty vector on failure or when no render endpoints are available.
std::vector<SourceDescriptor> enumerateRenderEndpoints() noexcept;

// Build a single render endpoint SourceDescriptor from raw data.
// Prepends "wasapi-loopback-" to the raw endpoint ID (T-03-02-01).
SourceDescriptor makeRenderEndpointDescriptor (const juce::String& rawEndpointId,
                                                const juce::String& displayName) noexcept;

} // namespace lumascope
