#pragma once

// Windows render endpoint notification hooks for standalone source lifecycle.
//
// These functions wrap an IMMNotificationClient that monitors eRender
// endpoint changes. The opaque handle (void*) pattern avoids exposing
// Windows SDK types in this header.
//
// Usage:
//   auto* handle = registerEndpointNotifications();
//   // ... call consumeEndpointNotificationFlags(handle) periodically
//   unregisterEndpointNotifications(handle);

namespace lumascope
{

// Bitmask constants for consumeEndpointNotificationFlags return value.
// These match the internal implementation in WasapiDeviceNotifications.cpp.
inline constexpr int kFlagEndpointAdded    = 1;
inline constexpr int kFlagEndpointRemoved  = 2;
inline constexpr int kFlagDefaultChanged   = 4;
inline constexpr int kFlagStateChanged     = 8;

// Register an IMMNotificationClient for eRender endpoint events.
// Returns an opaque handle, or nullptr on failure.
// Thread-safe once registered; flag consumption is atomic.
void* registerEndpointNotifications() noexcept;

// Unregister the notification client and release COM resources.
// Safe to call with nullptr.
void unregisterEndpointNotifications (void* handle) noexcept;

// Atomically consume and return accumulated notification flags.
// Returns a bitmask of: 1=added, 2=removed, 4=default-changed, 8=state-changed
// Returns 0 if no flags are set since last call.
int consumeEndpointNotificationFlags (void* handle) noexcept;

} // namespace lumascope
