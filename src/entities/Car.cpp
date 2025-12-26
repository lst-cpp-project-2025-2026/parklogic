#include "entities/Car.hpp"
#include "entities/map/World.hpp"
#include "raymath.h"
#include <memory>
#include <vector>

#include "config.hpp"
#include "core/AssetManager.hpp"

/**
 * @file Car.cpp
 * @brief Implementation of the Car entity.
 *
 * Handles physics integration, steering behaviors (acceleration, velocity),
 * collision avoidance, and state management (Driving, Parking, etc.).
 */

/**
 * @brief Constructs a new Car object.
 *
 * @param startPos The initial position of the car (in meters).
 * @param world Pointer to the world environment for boundary checking.
 */
Car::Car(Vector2 startPos, const World * /*world*/, Vector2 initialVelocity, CarType type)
    : position(startPos), velocity(initialVelocity), acceleration{0, 0}, maxSpeed(15.0f), maxForce(60.0f), type(type) {

  // Pick visual based on type
  int variant = GetRandomValue(1, 3);
  if (type == CarType::COMBUSTION) {
    textureName = "car1" + std::to_string(variant);
    batteryLevel = 0.0f; // Not valid for combustion
  } else {
    textureName = "car2" + std::to_string(variant);
    batteryLevel = (float)GetRandomValue(10, 90); // Random start battery
  }

  // Initialize rotation
  if (Vector2Length(velocity) > 0.1f) {
    currentRotation = atan2f(velocity.y, velocity.x) * RAD2DEG + 90.0f;
  }
} // 15 m/s (~54 km/h), 60 m/s^2 force

void Car::charge(float amount) {
  if (type == CarType::ELECTRIC) {
    batteryLevel += amount;
    if (batteryLevel > 100.0f)
      batteryLevel = 100.0f;
  }
}

void Car::setParkingContext(const Module *fac, const Spot &spot, int spotIndex) {
  parkedFacility = fac;
  parkedSpot = spot;
  parkedSpotIndex = spotIndex;
}

/**
 * @brief Updates the car's state based on time elapsed, without considering neighbors. Mostly useless.
 *
 * @param dt Time elapsed since the last update.
 */
void Car::update(double dt) { updateWithNeighbors(dt, nullptr); }

// In Car.cpp

/**
 * @brief Updates the car's state with awareness of neighbors.
 *
 * This function handles the core AI logic:
 * 1. State Management (Static states like PARKED).
 * 2. Path Following (Seek behavior).
 * 3. Collision Avoidance (Braking and Steering).
 * 4. Physics Integration (Velocity/Position updates).
 *
 * @param dt Delta time.
 * @param cars List of other cars for collision checks.
 */
void Car::updateWithNeighbors(double dt, const std::vector<std::unique_ptr<Car>> *cars) {
  // 1. Handle Static States
  if (state == CarState::PARKED) {
    parkingTimer -= (float)dt;
    return;
  }

  // 2. Path Following (Seek)
  if (!waypoints.empty()) {
    Waypoint &currentWp = waypoints.front();
    seek(currentWp);

    if (Vector2Distance(position, currentWp.position) < currentWp.tolerance) {
      if (waypoints.size() == 1) {
        if (currentWp.stopAtEnd && state == CarState::DRIVING) {
          velocity = {0, 0};
          acceleration = {0, 0};
          state = CarState::ALIGNING;
          targetRotation = currentWp.entryAngle;
        }
      }
      waypoints.pop_front();
    }
  } else {
    if (state == CarState::ALIGNING) {
      // Smooth Rotation Logic
      float targetDeg = (targetRotation * RAD2DEG) + 90.0f;
      float rotSpeed = 120.0f;
      float diff = targetDeg - currentRotation;
      while (diff > 180.0f)
        diff -= 360.0f;
      while (diff <= -180.0f)
        diff += 360.0f;

      if (fabs(diff) < 1.0f) {
        currentRotation = targetDeg;
        state = CarState::PARKED;
        parkingTimer =
            (float)GetRandomValue((int)(Config::PARKING_MIN_TIME * 10), (int)(Config::PARKING_MAX_TIME * 10)) / 10.0f;
      } else {
        float change = rotSpeed * (float)dt;
        if (change > fabs(diff))
          change = fabs(diff);
        currentRotation += (diff > 0) ? change : -change;
      }
      return;
    } else if (state == CarState::DRIVING) {
      velocity = Vector2Scale(velocity, 0.95f);
    }
  }

  // 3. Collision Avoidance (Enhanced to prevent Head-On Deadlocks)
  if (cars && (state == CarState::DRIVING || state == CarState::EXITING)) {
    // Fallback to rotation-based heading if velocity is zero to prevent getting stuck
    Vector2 heading = (Vector2Length(velocity) > 0.1f) ? Vector2Normalize(velocity)
                                                       : Vector2{cosf((currentRotation - 90.0f) * DEG2RAD),
                                                                 sinf((currentRotation - 90.0f) * DEG2RAD)};
    Vector2 sideVec = {-heading.y, heading.x};

    float currentSpeed = Vector2Length(velocity);
    float lookAheadDist = 5.0f + (currentSpeed * 1.5f);
    float laneWidth = 2.2f;

    for (const auto &other : *cars) {
      if (other.get() == this || other->state == CarState::PARKED)
        continue;

      Vector2 toOther = Vector2Subtract(other->getPosition(), position);
      float distSq = Vector2LengthSqr(toOther);
      if (distSq > lookAheadDist * lookAheadDist)
        continue;

      float dotForward = Vector2DotProduct(toOther, heading);
      float dotSide = Vector2DotProduct(toOther, sideVec);

      // If car is in the frontal corridor
      if (dotForward > 0 && dotForward < lookAheadDist && fabsf(dotSide) < laneWidth) {

        // A. Braking Logic
        float proximity = 1.0f - (dotForward / lookAheadDist);
        float brakingForce = 45.0f * (proximity * proximity);
        applyForce(Vector2Scale(heading, -brakingForce));

        // B. Deadlock Breaker (The Fix)
        // If we are facing each other or very slow, nudge to the side
        Vector2 otherHeading = (Vector2Length(other->velocity) > 0.1f)
                                   ? Vector2Normalize(other->velocity)
                                   : Vector2{cosf((other->currentRotation - 90.0f) * DEG2RAD),
                                             sinf((other->currentRotation - 90.0f) * DEG2RAD)};

        float alignment = Vector2DotProduct(heading, otherHeading);
        if (alignment < -0.3f || currentSpeed < 0.5f) {
          // Force a side-step: steer away from their position relative to us
          float steerDir = (dotSide > 0) ? -1.0f : 1.0f;
          applyForce(Vector2Scale(sideVec, 25.0f * steerDir));
        }

        // C. Match velocity
        float otherSpeed = Vector2Length(other->velocity);
        if (currentSpeed > otherSpeed) {
          float matchForce = (currentSpeed - otherSpeed) * 12.0f;
          applyForce(Vector2Scale(heading, -matchForce));
        }
      }

      // Lateral repulsion (keeps cars from overlapping)
      float dist = sqrtf(distSq);
      if (dist < 1.8f) {
        float pushStrength = 30.0f * (1.0f - (dist / 1.8f));
        applyForce(Vector2Scale(Vector2Normalize(toOther), -pushStrength));
      }
    }
  }

  // 4. Physics Integration
  if (state != CarState::PARKED && state != CarState::ALIGNING) {
    applyForce(Vector2Scale(velocity, -0.05f)); // Drag

    velocity = Vector2Add(velocity, Vector2Scale(acceleration, (float)dt));

    if (Vector2Length(velocity) > maxSpeed) {
      velocity = Vector2Scale(Vector2Normalize(velocity), maxSpeed);
    }

    position = Vector2Add(position, Vector2Scale(velocity, (float)dt));

    // 5. Smooth Rotation (Enhanced responsiveness)
    float speed = Vector2Length(velocity);
    if (speed > 0.05f) {
      float targetRot = atan2f(velocity.y, velocity.x) * RAD2DEG + 90.0f;
      float angleDiff = targetRot - currentRotation;
      while (angleDiff > 180)
        angleDiff -= 360;
      while (angleDiff < -180)
        angleDiff += 360;

      // Rotation speed scales slightly with velocity so cars don't spin in place
      float rotationFactor = (speed < 1.0f) ? 0.1f : 0.15f;
      currentRotation += angleDiff * rotationFactor;
    }
  }

  acceleration = {0, 0};
}

/**
 * @brief Draws the car, its velocity vector, and its current waypoints.
 */
void Car::draw() {
  // Draw Waypoints and paths (in Meters)
  if (!waypoints.empty()) {
    for (size_t i = 0; i < waypoints.size(); ++i) {
      Vector2 wpPos = waypoints[i].position;
      // Radius: 0.25 meters
      DrawCircleV(wpPos, 0.25f, Fade(BLUE, 0.5f));
      if (i > 0) {
        Vector2 prevWpPos = waypoints[i - 1].position;
        DrawLineV(prevWpPos, wpPos, Fade(BLUE, 0.3f));
      } else {
        DrawLineV(position, wpPos, Fade(BLUE, 0.3f));
      }
    }
  }

  // Draw Car
  Texture2D tex = AssetManager::Get().GetTexture(textureName);

  // Dimensions in Meters
  // Art pixel dimensions: 17 x 31
  float width = 17.0f / static_cast<float>(Config::ART_PIXELS_PER_METER);
  float height = 31.0f / static_cast<float>(Config::ART_PIXELS_PER_METER);

  // Rotation
  float rotation = currentRotation; // Use smoothed rotation

  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {position.x, position.y, width, height};
  Vector2 origin = {width / 2.0f, height / 2.0f};

  DrawTexturePro(tex, source, dest, origin, rotation, WHITE);

  // Draw velocity vector (heading) for debug
  // Vector2 velEnd = Vector2Add(position, Vector2Scale(velocity, 0.5f));
  // DrawLineV(position, velEnd, GREEN);
}

/**
 * @brief Adds a point to the list of waypoints the car should follow.
 *
 * @param wp The new waypoint.
 */
void Car::addWaypoint(Waypoint wp) { waypoints.push_back(wp); }

void Car::setPath(const std::vector<Waypoint> &path) {
  waypoints.clear();
  for (const auto &wp : path) {
    waypoints.push_back(wp);
  }
}

/**
 * @brief Clears all current waypoints.
 */
void Car::clearWaypoints() { waypoints.clear(); }

/**
 * @brief Applies a force vector to the car, accumulating in the acceleration vector.
 *
 * @param force The force vector to apply.
 */
void Car::applyForce(Vector2 force) { acceleration = Vector2Add(acceleration, force); }

/**
 * @brief Calculates the steering force required to move towards a target position (Seek behavior).
 *
 * @param target The target position to seek.
 */
void Car::seek(const Waypoint &wp) {
  Vector2 target = wp.position;
  Vector2 desired = Vector2Subtract(target, position);
  float dist = Vector2Length(desired);
  desired = Vector2Normalize(desired);

  // Calculate angle between current velocity and desired direction
  float currentAngle = atan2f(velocity.y, velocity.x);

  // Use the waypoint's entry angle if provided (and not 0/default), otherwise use direction to target
  // Actually, the user wants us to use the waypoint's turn angle info.
  // If entryAngle is set, it means "we should be at this angle when we hit the waypoint".
  // So we compare our current angle to that entry angle to decide if we need to slow down.

  // 1. Base Speed Limit from Waypoint (Segment Speed)
  float limitSpeed = maxSpeed * wp.speedLimitFactor;
  float speed = limitSpeed;

  // 2. Turn Slowdown Logic
  // Calculate angle difference between current heading and target heading
  float targetAngle = wp.entryAngle; // Is set for critical turns in PathPlanner

  float diff = currentAngle - targetAngle;
  while (diff <= -PI)
    diff += 2 * PI;
  while (diff > PI)
    diff -= 2 * PI;

  float angleDiff = fabs(diff);

  // If we are approaching a turn, slow down
  if (dist < Config::CarAI::TURN_SLOWDOWN_DIST) {
    if (angleDiff > Config::CarAI::TURN_SLOWDOWN_ANGLE) {
      // Factor: 0.0 (at waypoint) to 1.0 (at distance boundary)
      float factor = (dist / Config::CarAI::TURN_SLOWDOWN_DIST);

      float turnMinSpeed = maxSpeed * Config::CarAI::TURN_MIN_SPEED_FACTOR;

      // Interpolate: As we get closer (factor -> 0), speed drops towards turnMinSpeed
      float flexibleSpeed = turnMinSpeed + (limitSpeed - turnMinSpeed) * factor;

      // Apply if it requires slowing down (don't speed up if we are already slow)
      if (speed > flexibleSpeed) {
        speed = flexibleSpeed;
      }
    }
  }

  // Stop at end
  if (wp.stopAtEnd) {
    // Arrive behavior
    float stopRadius = 10.0f;
    if (dist < stopRadius) {
      float t = dist / stopRadius;
      // Linear ramp down from MaxSpeed
      float stopSpeed = maxSpeed * t;

      // Respect the segment limit and turn slowdown
      // If stopSpeed is higher than our current limited speed, ignore it (keep the lower speed)
      // If stopSpeed is lower (we are very close), use it to stop.
      if (stopSpeed < speed) {
        speed = stopSpeed;
      }

      if (speed < 0.5f)
        speed = 0.5f; // Min speed to actually reach it
    }
  }

  desired = Vector2Scale(desired, speed);

  Vector2 steer = Vector2Subtract(desired, velocity);

  // Limit the steering force to maxForce
  if (Vector2Length(steer) > maxForce) {
    steer = Vector2Scale(Vector2Normalize(steer), maxForce);
  }

  applyForce(steer);
}
