// SPDX-License-Identifier: MIT
//
// Declarations mirrored from ALVR's MIT-licensed alvr/server_core/src/c_api.rs;
// carries ALVR's license rather than the oxrsys-wide MPL-2.0.
//
// Hand-written C header for ALVR's alvr_server_core C API.
// Based on ALVR v20.14.1 with the dingyifei/ALVR testing extension for
// negotiated OXRSys frame pacing. This mirrors alvr/server_core/src/c_api.rs
// at that branch exactly. If the ALVR checkout moves, this header MUST be
// re-verified against c_api.rs before the embedded backend is built.
//
// ABI notes:
// - Rust #[repr(u8)] fieldless-variant tags are passed as uint8_t, so
//   AlvrCodecType/AlvrHandType are typedef'd to uint8_t here (a C enum would
//   pass as int and break the call ABI).
// - AlvrEvent is a #[repr(u8)] enum with payload variants. Per Rust's
//   RFC 2195 layout this is a union of per-variant structs where each
//   variant struct leads with the tag byte followed by its fields at their
//   natural alignment. It is mirrored below as exactly that union; do not
//   "simplify" it to struct { uint8_t tag; union {...} } (that would move
//   the ViewsConfig/PlayspaceSync payloads from offset 4 to offset 8).

#ifndef ALVR_SERVER_CORE_H
#define ALVR_SERVER_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AlvrFov {
    float left;  // negative, radians
    float right; // positive, radians
    float up;    // positive, radians
    float down;  // negative, radians
} AlvrFov;

typedef struct AlvrQuat {
    float x;
    float y;
    float z;
    float w;
} AlvrQuat;

typedef uint8_t AlvrCodecType;
enum {
    ALVR_CODEC_H264 = 0,
    ALVR_CODEC_HEVC = 1,
    ALVR_CODEC_AV1 = 2,
};

typedef struct AlvrPose {
    AlvrQuat orientation;
    float position[3];
} AlvrPose;

typedef struct AlvrDeviceMotion {
    AlvrPose pose;
    float linear_velocity[3];
    float angular_velocity[3];
} AlvrDeviceMotion;

typedef uint8_t AlvrHandType;
enum {
    ALVR_HAND_LEFT = 0,
    ALVR_HAND_RIGHT = 1,
};

typedef union AlvrButtonValue {
    bool scalar; // ButtonValue::Binary
    float float_; // ButtonValue::Scalar
} AlvrButtonValue;

// The interaction profile is implied.
typedef struct AlvrButtonEntry {
    uint64_t id; // hashed input path, see alvr_path_to_id
    AlvrButtonValue value;
} AlvrButtonEntry;

typedef struct AlvrBatteryInfo {
    uint64_t device_id;
    float gauge_value; // range [0, 1]
    bool is_plugged;
} AlvrBatteryInfo;

#define ALVR_SERVER_CORE_API_VERSION 1u
#define ALVR_FRAME_PACING_VERSION_1 1u
#define ALVR_FRAME_PACING_MODE_OFF 0u
#define ALVR_FRAME_PACING_MODE_SHADOW 1u
#define ALVR_FRAME_PACING_MODE_ACTIVE 2u
#define ALVR_FRAME_FEEDBACK_FLAG_FRESH 0x0001u
#define ALVR_FRAME_FEEDBACK_FLAG_SLACK_VALID 0x0002u

typedef struct AlvrFramePacingCapabilities {
    uint16_t supported_version;
    uint16_t negotiated_version;
    uint32_t reserved;
    uint64_t session_epoch;
} AlvrFramePacingCapabilities;

typedef struct AlvrFramePacingDisplayTiming {
    uint64_t session_epoch;
    uint64_t display_sequence;
    uint64_t predicted_display_time_ns;
    uint64_t predicted_display_period_ns;
} AlvrFramePacingDisplayTiming;

typedef struct AlvrFramePacingTimesyncResponse {
    uint64_t session_epoch;
    uint64_t query_sequence;
    uint64_t server_send_time_ns;
    uint64_t client_receive_time_ns;
    uint64_t client_send_time_ns;
} AlvrFramePacingTimesyncResponse;

typedef struct AlvrFramePacingFrameFeedback {
    uint64_t session_epoch;
    uint64_t feedback_sequence;
    uint64_t frame_timestamp_ns;
    uint64_t decode_complete_time_ns;
    uint64_t acquire_time_ns;
    uint64_t display_target_time_ns;
    uint64_t predicted_display_time_ns;
    int64_t acquire_slack_ns;
    uint32_t times_displayed;
    uint32_t flags;
} AlvrFramePacingFrameFeedback;

// Tag values follow the variant declaration order in c_api.rs.
typedef uint8_t AlvrEventTag;
enum {
    ALVR_EVENT_CLIENT_CONNECTED = 0,
    ALVR_EVENT_CLIENT_DISCONNECTED = 1,
    ALVR_EVENT_BATTERY = 2,
    ALVR_EVENT_PLAYSPACE_SYNC = 3,
    ALVR_EVENT_VIEWS_CONFIG = 4,
    ALVR_EVENT_TRACKING_UPDATED = 5,
    ALVR_EVENT_BUTTONS_UPDATED = 6,
    ALVR_EVENT_REQUEST_IDR = 7,
    ALVR_EVENT_CAPTURE_FRAME = 8,
    ALVR_EVENT_RESTART_PENDING = 9,
    ALVR_EVENT_SHUTDOWN_PENDING = 10,
    ALVR_EVENT_FRAME_PACING_DISPLAY_TIMING = 11,
    ALVR_EVENT_FRAME_PACING_TIMESYNC_RESPONSE = 12,
    ALVR_EVENT_FRAME_PACING_FRAME_FEEDBACK = 13,
};

typedef struct AlvrEventBattery {
    AlvrEventTag tag; // ALVR_EVENT_BATTERY
    AlvrBatteryInfo info; // at offset 8 (uint64_t alignment)
} AlvrEventBattery;

typedef struct AlvrEventPlayspaceSync {
    AlvrEventTag tag; // ALVR_EVENT_PLAYSPACE_SYNC
    float bounds[2]; // at offset 4 (float alignment)
} AlvrEventPlayspaceSync;

typedef struct AlvrEventViewsConfig {
    AlvrEventTag tag; // ALVR_EVENT_VIEWS_CONFIG
    AlvrPose local_view_transform[2]; // at offset 4
    AlvrFov fov[2];
} AlvrEventViewsConfig;

typedef struct AlvrEventTrackingUpdated {
    AlvrEventTag tag; // ALVR_EVENT_TRACKING_UPDATED
    uint64_t sample_timestamp_ns; // at offset 8
} AlvrEventTrackingUpdated;

typedef struct AlvrEventFramePacingDisplayTiming {
    AlvrEventTag tag; // ALVR_EVENT_FRAME_PACING_DISPLAY_TIMING
    AlvrFramePacingDisplayTiming timing; // at offset 8
} AlvrEventFramePacingDisplayTiming;

typedef struct AlvrEventFramePacingTimesyncResponse {
    AlvrEventTag tag; // ALVR_EVENT_FRAME_PACING_TIMESYNC_RESPONSE
    AlvrFramePacingTimesyncResponse response; // at offset 8
} AlvrEventFramePacingTimesyncResponse;

typedef struct AlvrEventFramePacingFrameFeedback {
    AlvrEventTag tag; // ALVR_EVENT_FRAME_PACING_FRAME_FEEDBACK
    AlvrFramePacingFrameFeedback feedback; // at offset 8
} AlvrEventFramePacingFrameFeedback;

typedef union AlvrEvent {
    AlvrEventTag tag; // valid for every variant
    AlvrEventBattery battery;
    AlvrEventPlayspaceSync playspace_sync;
    AlvrEventViewsConfig views_config;
    AlvrEventTrackingUpdated tracking_updated;
    AlvrEventFramePacingDisplayTiming frame_pacing_display_timing;
    AlvrEventFramePacingTimesyncResponse frame_pacing_timesync_response;
    AlvrEventFramePacingFrameFeedback frame_pacing_frame_feedback;
} AlvrEvent;

#ifdef __cplusplus
static_assert(sizeof(AlvrFramePacingCapabilities) == 16);
static_assert(sizeof(AlvrFramePacingDisplayTiming) == 32);
static_assert(sizeof(AlvrFramePacingTimesyncResponse) == 40);
static_assert(sizeof(AlvrFramePacingFrameFeedback) == 72);
static_assert(offsetof(AlvrEventFramePacingDisplayTiming, timing) == 8);
static_assert(offsetof(AlvrEventFramePacingTimesyncResponse, response) == 8);
static_assert(offsetof(AlvrEventFramePacingFrameFeedback, feedback) == 8);
static_assert(sizeof(AlvrEvent) == 96);
#endif

typedef struct AlvrTargetConfig {
    uint32_t game_render_width;
    uint32_t game_render_height;
    uint32_t stream_width;
    uint32_t stream_height;
} AlvrTargetConfig;

typedef struct AlvrDeviceConfig {
    uint64_t device_id;
    uint64_t interaction_profile_id;
} AlvrDeviceConfig;

typedef struct AlvrDynamicEncoderParams {
    float bitrate_bps;
    float framerate;
} AlvrDynamicEncoderParams;

// NOTE: upstream implements this as Instant::now().elapsed() which is ~0;
// do not rely on it. Timestamps fed back into alvr_send_video_nal /
// alvr_get_device_motion must come from TrackingUpdated events instead.
uint64_t alvr_get_time_ns(void);

// The testing extension uses API version 1. Refuse to start when this differs.
uint32_t alvr_get_server_core_api_version(void);
bool alvr_get_frame_pacing_capabilities(AlvrFramePacingCapabilities* out_capabilities);

// Must be called before alvr_initialize(). Selects the pacing mode:
// ALVR_FRAME_PACING_MODE_OFF/SHADOW/ACTIVE. OFF advertises pacing version 0, so no
// client leaves the legacy path; SHADOW/ACTIVE advertise version 1 and the negotiated
// mode is echoed to the client to distinguish telemetry-only from closed-loop selection.
void alvr_set_frame_pacing_mode(uint8_t mode);

// Hash an OpenXR-style path string (e.g. "/user/hand/left") to the id used
// by device/input/output functions.
uint64_t alvr_path_to_id(const char* path_string);

void alvr_error(const char* string);
void alvr_warn(const char* string);
void alvr_info(const char* string);
void alvr_dbg_server_impl(const char* string);
void alvr_dbg_encoder(const char* string);
void alvr_log_periodically(const char* tag, const char* message);

// Call with NULL to get the required buffer length (including NUL).
uint64_t alvr_get_settings_json(char* buffer);

// Must be called before alvr_initialize(). config_dir holds session.json.
void alvr_initialize_environment(const char* config_dir, const char* log_dir);

// Either path may be NULL (log goes to stdout/stderr).
void alvr_initialize_logging(const char* session_log_path, const char* crash_log_path);

AlvrTargetConfig alvr_initialize(void);

void alvr_start_connection(void);

// Returns true if an event was written to out_event within timeout_ns.
bool alvr_poll_event(AlvrEvent* out_event, uint64_t timeout_ns);

// Returns false if there is no tracking sample for the requested timestamp.
bool alvr_get_device_motion(uint64_t device_id,
                            uint64_t sample_timestamp_ns,
                            AlvrDeviceMotion* out_motion);

// out_skeleton must be an array of length 26.
bool alvr_get_hand_skeleton(AlvrHandType hand_type,
                            uint64_t sample_timestamp_ns,
                            AlvrPose* out_skeleton);

// Call with NULL to get the entry count; call with a buffer to pop the queue.
uint64_t alvr_get_buttons(AlvrButtonEntry* out_entries);

void alvr_send_haptics(uint64_t device_id, float duration_s, float frequency, float amplitude);

// buffer holds Annex-B parameter sets (SPS+PPS for H.264, VPS+SPS+PPS for HEVC).
void alvr_set_video_config_nals(AlvrCodecType codec, const uint8_t* buffer, int32_t len);

// Sends one clock-offset query on the negotiated pacing stream.
bool alvr_send_frame_pacing_timesync(uint64_t server_send_time_ns);

// One call per frame with the full Annex-B frame data. timestamp_ns remains the
// tracking/frame identity; display_target_time_ns is a separate client-clock
// display slot and may be 0 while the target servo is unsettled.
bool alvr_send_video_nal_paced(uint64_t timestamp_ns,
                                uint64_t display_target_time_ns,
                                const uint8_t* buffer,
                                int32_t len,
                                bool is_idr);

// Version-0 compatibility wrapper: equivalent to the paced call with target 0.
void alvr_send_video_nal(uint64_t timestamp_ns, uint8_t* buffer, int32_t len, bool is_idr);

// Returns true if params were updated since the last call.
bool alvr_get_dynamic_encoder_params(AlvrDynamicEncoderParams* out_params);

void alvr_report_composed(uint64_t timestamp_ns, uint64_t offset_ns);
void alvr_report_present(uint64_t timestamp_ns, uint64_t offset_ns);

// Returns true if a valid value was written.
bool alvr_duration_until_next_vsync(uint64_t* out_ns);

void alvr_restart(void);
void alvr_shutdown(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ALVR_SERVER_CORE_H
