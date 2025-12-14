#pragma once
#include "raylib.h"

struct Waypoint {
    Vector2 position; // Global position in meters
    float tolerance;  // Radius in meters to consider "reached"
    int id;           // Optional ID for identification
    float entryAngle; // Angle in radians to enter/leave this waypoint
    bool stopAtEnd;   // Whether to stop at this waypoint
    float speedLimitFactor; // Limit max speed for the segment ending at this waypoint (0.0 to 1.0)

    Waypoint(Vector2 pos, float tol = 1.0f, int _id = -1, float angle = 0.0f, bool stop = false, float speedFactor = 1.0f) 
        : position(pos), tolerance(tol), id(_id), entryAngle(angle), stopAtEnd(stop), speedLimitFactor(speedFactor) {}
};
