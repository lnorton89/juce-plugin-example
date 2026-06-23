#pragma once

#include "LumaScope/Standalone/SourceModel.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

// Forward declarations for Windows COM types (full definition only in .cpp).
struct IMMDevice;
struct IAudioClient;
struct IAudioCaptureClient;

class LumaScopeAudioProcessor;

namespace lumascope
{

// ============================================================================
// WASAPI format conversion utilities.
// These convert raw WASAPI capture packets into finite float buffers for
// the analyzer ingress. All functions are designed for bounded, preallocated
// use on a capture thread (no internal allocations).
//
// The WAVEFORMATEX parameter is passed as void* to avoid requiring
// Windows SDK headers in includers of this header.
// ============================================================================
namespace WasapiConversion
{

// Maximum supported output channels after downmix
constexpr int maxOutputChannels = 2;

// Convert a 32-bit IEEE float WASAPI packet to a float buffer.
// Channels are in interleaved format (sample[n] = channel n % numChannels).
void convertFloat32 (const std::uint8_t* source, float* const* destChannels,
                     int numSourceChannels, int numDestChannels,
                     int frameCount) noexcept;

// Convert a 16-bit signed integer PCM WASAPI packet to a float buffer.
void convertInt16 (const std::uint8_t* source, float* const* destChannels,
                   int numSourceChannels, int numDestChannels,
                   int frameCount) noexcept;

// Convert a 24-bit signed integer PCM WASAPI packet (3 bytes per sample)
// to a float buffer.
void convertInt24 (const std::uint8_t* source, float* const* destChannels,
                   int numSourceChannels, int numDestChannels,
                   int frameCount) noexcept;

// Convert a 32-bit signed integer PCM WASAPI packet to a float buffer.
void convertInt32 (const std::uint8_t* source, float* const* destChannels,
                   int numSourceChannels, int numDestChannels,
                   int frameCount) noexcept;

// Dispatch conversion by WAVEFORMATEX format.
// source: interleaved WASAPI packet data (already extracted via GetBuffer).
// destBuffer: preallocated output buffer.
// Returns true on success, false on unsupported format.
// format is const void* pointing to a WAVEFORMATEX struct.
bool convertPacket (const std::uint8_t* source, int frameCount,
                    const void* format,
                    juce::AudioBuffer<float>& destBuffer) noexcept;

// Compute the total audio level in a buffer (sum of absolute values / samples).
// Used for silence detection.
double computeLevel (const juce::AudioBuffer<float>& buffer) noexcept;

} // namespace WasapiConversion

// ============================================================================
// WasapiLoopbackSourceAdapter: shared-mode WASAPI loopback capture on a
// render endpoint.
//
// Real-time safety: all COM setup, format allocation, and recovery decisions
// occur outside the capture thread. The capture thread only reads packets,
// converts within preallocated bounds, updates atomics, and feeds the
// analyzer ingress.
// ============================================================================
class WasapiLoopbackSourceAdapter
{
public:
    explicit WasapiLoopbackSourceAdapter (LumaScopeAudioProcessor& processor) noexcept;
    ~WasapiLoopbackSourceAdapter();

    // Open the specified render endpoint and start loopback capture.
    // Returns empty string on success, error description on failure.
    juce::String start (const juce::String& endpointId);

    // Stop capture and release all COM resources.
    void stop();

    // Check if capture is currently running.
    bool isRunning() const noexcept { return running.load (std::memory_order_acquire); }

    // Get current capture state based on silence detection.
    SourceState currentCaptureState() const noexcept;

    // Reset silence detection state (e.g., after source switch).
    void resetSilenceDetection() noexcept;

    // Number of consecutive silent frames before reporting 'silent' state.
    static constexpr int silentFramesThreshold = 20;

private:
    // The capture thread function.
    void captureThreadFunc (IMMDevice* device);

    // Ensure scratch buffer is large enough.
    void ensureScratchCapacity (int samples);

    LumaScopeAudioProcessor& processor;

    // COM interfaces (owned by this adapter, released in stop())
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    void* mixFormat = nullptr; // WAVEFORMATEX* (cast in .cpp)

    // Thread management
    std::unique_ptr<std::thread> captureThread;
    std::atomic<bool> shouldStop { false };
    std::atomic<bool> running { false };
    void* captureEvent = nullptr; // HANDLE (cast in .cpp)

    // Conversion scratch buffer (preallocated, bounded resize only outside hot path)
    juce::AudioBuffer<float> scratch;

    // Silence detection atomics (written by capture thread, read by controller)
    std::atomic<double> recentLevel { 0.0 };
    mutable std::atomic<int> consecutiveSilentFrames { 0 };
};

} // namespace lumascope
