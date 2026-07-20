# Embedded ALVR Backend

## Scope

The `OXRSYS_ENABLE_ALVR` build option embeds the pinned `alvr_server_core` fork and lets the runtime stream to an ALVR v20.14.1 Quest client with `protocol = "alvr"`. OXRSys continues to own application-facing OpenXR, Metal frame capture, VideoToolbox encoding, and input translation. ALVR owns discovery, trust, transport, headset decode, and audio.

The fork keeps the base ALVR protocol ID at `20`. An unmodified v20.14.1 client therefore remains compatible and uses OXRSys's bounded absolute-deadline fallback grid.

## Frame-pacing extension

The enhanced client and embedded server negotiate `oxrsys_frame_pacing_version` through ALVR's existing JSON capability fields. Missing fields mean version `0`; version `1` is selected only when both endpoints advertise it. Existing positional ALVR packets are not changed.

Version 1 uses separate high-range streams:

| Stream | Direction | Payload |
|---|---|---|
| `0x8000` | both | clock-sync query/response |
| `0x8001` | client to server | predicted display time and period |
| `0x8002` | server to client | frame identity plus intended display target |
| `0x8003` | client to server | decode/acquire/display outcome |

The existing ALVR video timestamp remains the frame and tracking identity. The intended display target is carried separately so statistics and pose matching keep their v20 meaning.

Client timestamps use the Quest OpenXR time domain explicitly mapped to the client's monotonic connection clock. Server timestamps use `std::chrono::steady_clock`. Clock-sync responses are matched to outstanding sequence/timestamp pairs before OXRSys feeds them to `ClockOffsetEstimator`.

## Runtime modes

`[streaming].alvr_frame_pacing` accepts:

- `"off"`: advertise pacing version 0. No client — stock or enhanced — leaves its legacy path, and the server keeps the fallback release grid.
- `"shadow"` (default): advertise version 1 and negotiate the mode as shadow. The client reports clock/display/frame telemetry but keeps its legacy decoder dequeue path, and the server keeps the fallback release grid. This trains the pacer for A/B comparison without changing observable frame behavior.
- `"on"`: advertise version 1 and negotiate the mode as active. The server uses closed-loop release after clock stabilization and stamps display targets; the client latches frames against those targets and drives the adaptive acquire deadline.

The negotiated mode byte travels in the JSON stream config alongside the version, so the enhanced client distinguishes shadow (telemetry only) from active (target-aware latching). All modes fall back automatically when the client negotiates version 0, disconnects, or loses timing confidence. Reconnects use a new session epoch and clear timing/target history. Predicted OpenXR display times are clamped monotonic across mode transitions.

## Non-blocking requirements

- `Session::EndFrame()` remains non-blocking.
- Video targets travel through the existing bounded latest-frame queue.
- ALVR socket writes remain on sender threads, not the VideoToolbox callback or XR frame loop.
- Client timing and feedback queues are bounded and use non-blocking submission; missing or raced target metadata clears the valid-slack flag instead of guessing.
- FramePacer only consumes acquire slack when the client attributes it unambiguously to the intended display slot.

## Rosetta encoder contract

The Wine path intentionally keeps NV12 input, VideoToolbox low-latency rate control, `RealTime = true`, no frame reordering, and H.264 under Rosetta. PR #22's experimental `RealTime = false` encoder setting is not part of the ALVR pacing port and must be evaluated separately.

## Build and validation

The embedded server must be built from the matching ALVR fork commit; the hand-written C header in `runtime/src/alvr/alvr_server_core.h` is ABI-versioned and has fixed layout assertions.

Validate changes with:

```bash
cargo test --manifest-path ../ALVR/Cargo.toml \
  -p alvr_packets -p alvr_client_core -p alvr_server_core

cmake -S . -B build-x64 -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 -DOXRSYS_ENABLE_ALVR=ON
cmake --build build-x64
ctest --test-dir build-x64 --output-on-failure
```

Build the enhanced headset APK from the matching ALVR checkout. A differently signed APK may require uninstalling the Store/release client; preserve headset state and obtain confirmation before doing that.
