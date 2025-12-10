#include "entities/Car.hpp"
#include "entities/map/World.hpp"
#include "raymath.h"
#include <memory>
#include <vector>

#include "config.hpp"
#include "core/AssetManager.hpp"

/**
 * @brief Constructs a new Car object.
 *
 * @param startPos The initial position of the car (in meters).
 * @param world Pointer to the world environment for boundary checking.
 */
Car::Car(Vector2 startPos, const World *world, Vector2 initialVelocity)
    : position(startPos), velocity(initialVelocity), acceleration{0, 0}, world(world), maxSpeed(15.0f), maxForce(60.0f) {
    
    // Pick a random texture (car11, car12, car13)
    int type = GetRandomValue(1, 3);
    textureName = "car1" + std::to_string(type);
    
    // Initialize rotation
    if (Vector2Length(velocity) > 0.1f) {
        currentRotation = atan2f(velocity.y, velocity.x) * RAD2DEG + 90.0f;
    }
} // 15 m/s (~54 km/h), 60 m/s^2 force

void Car::setParkingContext(const Module* fac, const Spot& spot) {
    parkedFacility = fac;
    parkedSpot = spot;
}

/**
 * @brief Updates the car's state based on time elapsed, without considering neighbors.
 *
 * @param dt Time elapsed since the last update.
 */
void Car::update(double dt) { updateWithNeighbors(dt, nullptr); }

// In Car.cpp

/**
 * @brief Updates the car's state with awareness of other cars.
 *
 * @param dt Delta time in seconds.
 * @param cars Pointer to the list of other cars for collision avoidance.
 */
void Car::updateWithNeighbors(double dt, const std::vector<std::unique_ptr<Car>> *cars) {
  // If we are finished exiting (reached map edge or end of path), dispatch should handle cleanup.
  // But here we just check if path is complete.
  
  if (state == CarState::PARKED) {
      parkingTimer -= (float)dt;
      return; 
  }

  // --- Steering Behavior (Seek) ---
  if (!waypoints.empty()) {
    Waypoint &currentWp = waypoints.front();
    seek(currentWp);

    // Check if reached (using waypoint's tolerance)
    if (Vector2Distance(position, currentWp.position) < currentWp.tolerance) {
      
      // If we reached the last waypoint
      if (waypoints.size() == 1) {
          // Check if this was a parking spot (Stop at end)
          if (currentWp.stopAtEnd && state == CarState::DRIVING) {
              // We arrived at spot.
              velocity = {0,0};
              acceleration = {0,0};
              state = CarState::ALIGNING;
              targetRotation = currentWp.entryAngle; // The spot's orientation
          } else if (state == CarState::EXITING) {
              // Reached exit point (Map edge)
              // The TrafficSystem should likely handle despawn, but we can flag it?
              // For now, just stop.
              velocity = {0,0};
          }
      }
      waypoints.pop_front();
    }
  } else {
    // No path
    // If we are ALIGNING, we are sitting in the spot wanting to rotate.
    if (state == CarState::ALIGNING) {
        // Smooth Rotation Logic
        float targetDeg = (targetRotation * RAD2DEG) + 90.0f;
        float rotSpeed = 120.0f; // degrees per second
        
        float diff = targetDeg - currentRotation;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff <= -180.0f) diff += 360.0f;
        
        if (fabs(diff) < 1.0f) {
            currentRotation = targetDeg;
            state = CarState::PARKED;
            parkingTimer = 10.0f; // 10 seconds wait
        } else {
            float change = rotSpeed * (float)dt;
            if (change > fabs(diff)) change = fabs(diff);
            currentRotation += (diff > 0) ? change : -change;
        }
        
    } 
    else if (state == CarState::DRIVING) {
        // Just stopped?
        velocity = Vector2Scale(velocity, 0.95f);
    }
     else if (state == CarState::EXITING) {
         // Despawn logic triggers externally
    }
  }
  
  // Update rotation from velocity if moving
  if (state != CarState::PARKED && state != CarState::ALIGNING && Vector2Length(velocity) > 0.1f) {
      currentRotation = atan2f(velocity.y, velocity.x) * RAD2DEG + 90.0f;
  }

  // --- Collision Avoidance (Braking and Separation) ---
  // Only apply if moving and not parked
  if (state == CarState::DRIVING || state == CarState::EXITING) {
      if (cars) {
        for (const auto &other : *cars) {
          if (other.get() == this)
            continue;

          float dist = Vector2Distance(position, other->getPosition());

          // If a car is within the detection range (3.5m), apply avoidance forces
          if (dist < 3.5f) {
            // 1. Apply Braking (Force opposite to current velocity)
            if (Vector2Length(velocity) > 0.5f) {
              Vector2 heading = Vector2Normalize(velocity);
              float brakingStrength = 30.0f;
              Vector2 brakingForce = Vector2Scale(heading, -brakingStrength);
              applyForce(brakingForce);
            }

            // 2. Apply Separation (Push the car away from the neighbor)
            Vector2 push = Vector2Subtract(position, other->getPosition());
            push = Vector2Normalize(push);

            // Strength of the push increases as distance decreases
            float pushStrength = 25.0f * (1.0f - (dist / 3.5f));
            applyForce(Vector2Scale(push, pushStrength));
          }
        }
      }
  }

  // --- Physics Integration ---
  if (state != CarState::PARKED && state != CarState::ALIGNING) {
      // Apply accumulated acceleration to velocity
      velocity = Vector2Add(velocity, Vector2Scale(acceleration, (float)dt));

      // Limit velocity to max speed
      if (Vector2Length(velocity) > maxSpeed) {
        velocity = Vector2Scale(Vector2Normalize(velocity), maxSpeed);
      }

      // Update Position
      position = Vector2Add(position, Vector2Scale(velocity, (float)dt));
  }

  // Reset acceleration for the next frame
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

  float targetAngle = wp.entryAngle;
  // If entryAngle is 0, it might be uninitialized or actually 0.
  // Let's assume if it's a significant turn, it will be set.
  // But wait, we need to know the difference between current heading and the *next* leg's heading.
  // The user said: "each waypoint carries the information of the turn angle so the car can slow down depending on the
  // upcoming turn"

  float angleDiff = 0.0f;

  // If we have a specific entry angle for this waypoint (meaning a turn is expected)
  // We compare our current heading with that angle.
  // However, usually we want to slow down if the *next* turn is sharp.
  // The waypoint we are seeking IS the turn point.
  // So wp.entryAngle should represent the direction we need to be facing *after* the waypoint, or the direction of the
  // turn. Let's assume wp.entryAngle is the angle of the segment *entering* the facility or the *next* road segment.

  // Let's use the difference between current velocity angle and the waypoint's entry angle.
  float diff = fabs(currentAngle - targetAngle);
  while (diff > PI)
    diff -= 2 * PI;
  angleDiff = fabs(diff);

  float speed = maxSpeed;

  // Slow down for turns
  // If the waypoint has a specific angle (e.g. not just 0 or we rely on the flag)
  // For now, let's assume if angleDiff is large, we slow down.
  // But we only want to do this if we are getting close?
  // The user said "slow down depending on the upcoming turn instead of during a turn".
  // So as we approach the waypoint, if it's a sharp turn, we slow down.

  if (dist < 10.0f) {                // Start slowing down 10m before
    if (angleDiff > 0.5f) {          // > ~30 degrees
      float factor = (dist / 10.0f); // 0 to 1
      // Blend between maxSpeed and a slower turn speed
      float turnSpeed = maxSpeed * 0.4f;
      speed = turnSpeed + (maxSpeed - turnSpeed) * factor;
    }
  }

  // Stop at end
  if (wp.stopAtEnd) {
    // Arrive behavior
    float stopRadius = 10.0f;
    if (dist < stopRadius) {
      float t = dist / stopRadius;
      // Smooth step or linear
      speed = maxSpeed * t;
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
