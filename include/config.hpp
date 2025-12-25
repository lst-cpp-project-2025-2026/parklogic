#pragma once

namespace Config {
constexpr const char *WINDOW_TITLE = "ParkLogic"; ///< Window Title

// Logical Resolution (Render Texture Size)
constexpr int LOGICAL_WIDTH = 1600;      ///< Internal resolution width
constexpr int LOGICAL_HEIGHT = 900;      ///< Internal resolution height
constexpr int PIXELS_PER_ART_PIXEL = 4;  ///< Resolution pixels per art pixel
constexpr int ART_PIXELS_PER_METER = 7;  ///< Art pixels per meter
constexpr int BACKGROUND_TILE_SIZE = 32; ///< Background tile size in art pixels
constexpr float PPM = static_cast<float>(PIXELS_PER_ART_PIXEL * ART_PIXELS_PER_METER); // Pixels Per Meter (28.0f)

constexpr int LANE_OFFSET_UP = 61;   ///< Up lane offset from top of road (art pixels)
constexpr int LANE_OFFSET_DOWN = 94; ///< Down lane offset from top of road (art pixels)

// Physical Window Start Size
constexpr int INITIAL_WINDOW_WIDTH = 1280; ///< Initial window width
constexpr int INITIAL_WINDOW_HEIGHT = 720; ///< Initial window height

constexpr int TICK_RATE = 60;                                             ///< Fixed update rate (ticks per second)
constexpr double FIXED_DELTA_TIME = 1.0 / static_cast<double>(TICK_RATE); ///< Time per tick

constexpr int TARGET_FPS = 60;       ///< Target frames per second
constexpr bool VSYNC_ENABLED = true; ///< Vertical sync flag

namespace CarAI {
struct AIPhase {
  float speedFactor;    // % of max speed
  float tolerance;      // Waypoint clearance radius
  float correctionStep; // Distance between correction points (Meters)
};

namespace Phases {
constexpr AIPhase HIGHWAY = {1.0f, 10.0f, 20.0f}; // Fast, loose, corrections every 15m
constexpr AIPhase APPROACH = {1.0f, 3.5f, 5.0f};  // Approaches to facility
constexpr AIPhase ACCESS = {0.4f, 2.0f, 3.0f};    // Entry roads / Connector
constexpr AIPhase MANEUVER = {0.2f, 2.0f, 2.0f};  // Alignment / Interior
constexpr AIPhase PARKING = {0.1f, 0.3f, 3.0f};   // Final spot (corrected back to user pref)
} // namespace Phases

namespace GateDepth {
// Distance (Meters) to drive "into" the facility before aligning
constexpr float SMALL_PARKING = 12.0f;
constexpr float LARGE_PARKING = 12.0f;
constexpr float SMALL_CHARGING = 5.0f;
constexpr float LARGE_CHARGING = 12.0f;
constexpr float GENERIC = 5.0f;
} // namespace GateDepth

// Turn Logic
constexpr float TURN_SLOWDOWN_DIST = 30.0f;    // Start slowing down X meters before a sharp turn
constexpr float TURN_SLOWDOWN_ANGLE = 0.2f;    // Angle (radians) to consider "sharp" (~11 degrees)
constexpr float TURN_MIN_SPEED_FACTOR = 0.20f; // Slow down to at least this factor during turns
} // namespace CarAI

// Battery Constants
constexpr float BATTERY_LOW_THRESHOLD = 30.0f;
constexpr float BATTERY_HIGH_THRESHOLD = 70.0f;
constexpr float BATTERY_EXIT_THRESHOLD = 80.0f;       // Chance to leave
constexpr float BATTERY_FORCE_EXIT_THRESHOLD = 95.0f; // Must leave
constexpr float CHARGING_RATE = 0.25f;                // % per second

// Parking Timers (Seconds)
constexpr float PARKING_MIN_TIME = 120.0f;
constexpr float PARKING_MAX_TIME = 300.0f;

namespace Spawner {
// Spawn Intervals in Seconds (Real-time seconds at 1x speed, scaling with speed)
// Level 0: Off
// Level 1: Very Slow
// Level 2: Slow
// Level 3: Moderate
// Level 4: Fast
// Level 5: Very Fast
constexpr float SPAWN_RATES[] = {0.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f};
} // namespace Spawner
} // namespace Config
