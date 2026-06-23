#pragma once

#include "LumaScope/Standalone/SourceModel.h"

namespace lumascope
{

// Single native authority for standalone source lifecycle.
// Owns enumeration, selection, stop, and state observation.
// This controller is standalone-only; VST3 must not instantiate it.
class StandaloneSourceController
{
public:
    virtual ~StandaloneSourceController() noexcept = default;

    // Enumerate all available sources.
    // Returns separate lists for InputDevice and SystemOutput modes (D-04).
    virtual SourceList enumerateSources() = 0;

    // Select a source by its mode and ID.
    // Tears down any previously active source before starting the new one.
    // Returns the current state snapshot after the selection attempt.
    virtual SourceStateSnapshot selectSource (const SourceSelection& selection) = 0;

    // Stop the currently active source.
    // Returns to stopped state with no selected source.
    // Safe to call when already stopped.
    virtual SourceStateSnapshot stop() = 0;

    // Get the current source state snapshot without side effects.
    virtual SourceStateSnapshot currentStateSnapshot() const = 0;
};

} // namespace lumascope
