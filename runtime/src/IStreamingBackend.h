// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <string>

#include "GraphicsTypes.h"

class TrackingReceiver;

struct BackendFrameRelease
{
    int64_t displayTimeServerNs = 0;
    int64_t periodNs = 0;
};

/**
 * Interface between Session and a streaming backend implementation.
 *
 * Implementations:
 *  - StreamingServer: the oxrsys wire protocol (own Quest/Apple/Qt clients)
 *  - AlvrStreamingBackend: ALVR's embedded server_core (stock ALVR clients)
 *
 * Selected via `protocol = "oxrsys" | "alvr"` in oxrsys-runtime.toml.
 */
class IStreamingBackend
{
public:
    virtual ~IStreamingBackend() = default;

    virtual bool Start(uint32_t renderWidth, uint32_t renderHeight, uint32_t refreshRateHz) = 0;
    virtual void Stop() = 0;

    // Stop() variant for process exit / dylib unload: must still join the
    // backend's own threads but skip teardown that blocks on external runtimes
    // (ALVR's alvr_shutdown() joins a tokio runtime and waits for the client to
    // disconnect — fatal/hangy in a destructor at exit). Backends without such
    // state just Stop().
    virtual void StopForProcessExit() { Stop(); }

    // Queue a rendered frame for asynchronous latest-frame-only encoding.
    //
    // renderHeadOrientation (xyzw) / renderHeadPosition (xyz) are the exact head pose the
    // application rendered this frame for (from xrLocateViews). Backends that tag frames
    // with a pose should prefer it over a later re-prediction; pass nullptr when the
    // render pose is unavailable.
    virtual void SendFrame(FrameSource frameSource,
                           const float* renderHeadOrientation = nullptr,
                           const float* renderHeadPosition = nullptr) = 0;

    virtual void SetGraphicsContext(const GraphicsContext& graphicsContext) = 0;

    virtual bool IsClientConnected() const = 0;
    virtual uint32_t GetTargetRefreshRateHz() const = 0;
    virtual std::string GetClientName() const = 0;

    // Pose source consumed by InputManager (InjectPacket seam).
    virtual TrackingReceiver* GetTrackingReceiver() = 0;

    // Optional capabilities; backends without support keep the defaults.
    virtual void ApplyHaptics(int /*hand*/, float /*amplitude*/, float /*durationSeconds*/,
                              float /*frequencyHz*/)
    {
    }

    // Native OXRSys clients feed PR #22's display-clock FramePacer directly.
    // Backends that do not have closed-loop timing stay on the bounded absolute
    // grid below until they negotiate an equivalent timing extension.
    virtual bool UsesClosedLoopFramePacer() const { return false; }
    virtual void SetFramePacer(class FramePacer* /*framePacer*/) {}

    // A backend-owned closed-loop pacer may block until its next release and
    // return the target display tick. False keeps Session on its fallback grid.
    virtual bool WaitForFrameRelease(int64_t /*nowServerNs*/, int64_t /*nominalPeriodNs*/,
                                     BackendFrameRelease& /*outRelease*/)
    {
        return false;
    }

    // Open-loop phase hint (nanoseconds until the next estimated client vsync).
    // Session uses this only to anchor its fallback grid, never to re-anchor each frame.
    virtual bool GetFramePacing(int64_t& /*outSleepNs*/) { return false; }
};
