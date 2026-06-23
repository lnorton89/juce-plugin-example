#pragma once

#include "LumaScope/Analyzer/SpectrumSnapshot.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace lumascope
{
class SnapshotMailbox
{
public:
    SnapshotMailbox() = default;
    SnapshotMailbox (const SnapshotMailbox&) = delete;
    SnapshotMailbox& operator= (const SnapshotMailbox&) = delete;
    SnapshotMailbox (SnapshotMailbox&&) = delete;
    SnapshotMailbox& operator= (SnapshotMailbox&&) = delete;

    void clear() noexcept
    {
        activeSlot.store (0, std::memory_order_release);
        publishedSequence.store (0, std::memory_order_release);
    }

    bool publish (const SpectrumSnapshot& snapshot) noexcept
    {
        if (! isValidSnapshot (snapshot))
            return false;

        const auto nextSlot = 1u - activeSlot.load (std::memory_order_relaxed);
        slots[nextSlot] = snapshot;
        activeSlot.store (nextSlot, std::memory_order_release);
        publishedSequence.store (snapshot.sequence, std::memory_order_release);
        return true;
    }

    bool readLatest (SpectrumSnapshot& destination, std::uint32_t& lastSeenSequence) const noexcept
    {
        const auto sequenceBefore = publishedSequence.load (std::memory_order_acquire);

        if (sequenceBefore == 0 || sequenceBefore == lastSeenSequence)
            return false;

        const auto slot = activeSlot.load (std::memory_order_acquire);
        destination = slots[slot];

        const auto sequenceAfter = publishedSequence.load (std::memory_order_acquire);
        if (sequenceBefore != sequenceAfter || destination.sequence != sequenceAfter)
            return false;

        lastSeenSequence = sequenceAfter;
        return true;
    }

private:
    static bool isFinite (float value) noexcept
    {
        return std::isfinite (value);
    }

    static bool isFinite (double value) noexcept
    {
        return std::isfinite (value);
    }

    static bool isValidSnapshot (const SpectrumSnapshot& snapshot) noexcept
    {
        if (snapshot.sequence == 0
            || ! isFinite (snapshot.sampleRate)
            || snapshot.sampleRate <= 0.0
            || snapshot.fftSize == 0
            || ! isFinite (snapshot.minFrequencyHz)
            || ! isFinite (snapshot.maxFrequencyHz)
            || snapshot.minFrequencyHz <= 0.0
            || snapshot.maxFrequencyHz <= snapshot.minFrequencyHz
            || ! isFinite (snapshot.minDecibels)
            || ! isFinite (snapshot.maxDecibels)
            || snapshot.minDecibels >= snapshot.maxDecibels
            || snapshot.binCount == 0
            || snapshot.binCount > SpectrumSnapshot::maxBins)
            return false;

        for (std::size_t index = 0; index < snapshot.binCount; ++index)
        {
            const auto& bin = snapshot.bins[index];
            if (! isFinite (bin.frequencyHz)
                || ! isFinite (bin.decibels)
                || ! isFinite (bin.normalisedValue)
                || bin.normalisedValue < 0.0f
                || bin.normalisedValue > 1.0f)
                return false;
        }

        return true;
    }

    std::array<SpectrumSnapshot, 2> slots {};
    std::atomic<unsigned> activeSlot { 0 };
    std::atomic<std::uint32_t> publishedSequence { 0 };
};
}
