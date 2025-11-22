#include "entities/Car.hpp"
#include "entities/World.hpp"
#include "raymath.h"
#include <memory>
#include <vector>

/**
 * @brief Constructs a new Car object.
 *
 * @param startPos The initial position of the car.
 * @param world Pointer to the world environment for boundary checking.
 */
Car::Car(Vector2 startPos, const World *world)
    : position(startPos), velocity{0, 0}, acceleration{0, 0}, world(world), maxSpeed(300.0f), maxForce(800.0f) {}

/**
 * @brief Updates the car's state based on time elapsed, without considering neighbors.
 *
 * @param dt Time elapsed since the last update.
 */
void Car::update(double dt) { updateWithNeighbors(dt, nullptr); }

/**
 * @brief Updates the car's state, including steering, physics, and collision avoidance.
 *
 * @param dt Time elapsed since the last update.
 * @param cars A pointer to the list of all cars in the scene, used for collision avoidance (can be nullptr).
 */
void Car::updateWithNeighbors(double dt, const std::vector<std::unique_ptr<Car>> *cars) {
  // --- Waypoint Logic: Generate a new random waypoint if none exist ---
  if (waypoints.empty()) {
    if (world) {
      float dist = (float)GetRandomValue(300, 500);
      float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
      Vector2 offset = {cosf(angle) * dist, sinf(angle) * dist};
      Vector2 nextPoint = Vector2Add(position, offset);

      // Clamp waypoint to stay within world bounds (with a 50px margin)
      if (nextPoint.x < 50)
        nextPoint.x = 50;
      if (nextPoint.y < 50)
        nextPoint.y = 50;
      if (nextPoint.x > world->getWidth() - 50)
        nextPoint.x = world->getWidth() - 50;
      if (nextPoint.y > world->getHeight() - 50)
        nextPoint.y = world->getHeight() - 50;

      addWaypoint(nextPoint);
    }
  }

  // --- Steering Behavior (Seek) ---
  if (!waypoints.empty()) {
    Vector2 target = waypoints.front();
    seek(target);

    // If close enough to the target, move to the next waypoint
    if (Vector2Distance(position, target) < 50.0f) {
      waypoints.pop_front();
    }
  } else {
    // Apply friction/drag when no target is set
    velocity = Vector2Scale(velocity, 0.95f);
  }

  // --- Collision Avoidance (Braking and Separation) ---
  if (cars) {
    for (const auto &other : *cars) {
      if (other.get() == this)
        continue;

      float dist = Vector2Distance(position, other->getPosition());

      // If a car is within the detection range, apply avoidance forces
      if (dist < 70.0f) {
        // 1. Apply Braking (Force opposite to current velocity)
        if (Vector2Length(velocity) > 10.0f) {
          Vector2 heading = Vector2Normalize(velocity);
          float brakingStrength = 600.0f;
          Vector2 brakingForce = Vector2Scale(heading, -brakingStrength);
          applyForce(brakingForce);
        }

        // 2. Apply Separation (Push the car away from the neighbor)
        Vector2 push = Vector2Subtract(position, other->getPosition());
        push = Vector2Normalize(push);

        // Strength of the push increases as distance decreases
        float pushStrength = 500.0f * (1.0f - (dist / 70.0f));
        applyForce(Vector2Scale(push, pushStrength));
      }
    }
  }

  // --- Physics Integration ---
  // Apply accumulated acceleration to velocity
  velocity = Vector2Add(velocity, Vector2Scale(acceleration, (float)dt));

  // Limit velocity to max speed
  if (Vector2Length(velocity) > maxSpeed) {
    velocity = Vector2Scale(Vector2Normalize(velocity), maxSpeed);
  }

  // Update Position
  position = Vector2Add(position, Vector2Scale(velocity, (float)dt));

  // Reset acceleration for the next frame
  acceleration = {0, 0};
}

/**
 * @brief Draws the car, its velocity vector, and its current waypoints.
 */
void Car::draw() {
  // Draw Waypoints and paths
  if (!waypoints.empty()) {
    for (size_t i = 0; i < waypoints.size(); ++i) {
      DrawCircleV(waypoints[i], 5, Fade(BLUE, 0.5f));
      if (i > 0) {
        DrawLineV(waypoints[i - 1], waypoints[i], Fade(BLUE, 0.3f));
      } else {
        DrawLineV(position, waypoints[i], Fade(BLUE, 0.3f));
      }
    }
  }

  // Draw Car rectangle
  float rotation = atan2f(velocity.y, velocity.x) * RAD2DEG;
  Rectangle carRect = {position.x, position.y, 40, 20};
  DrawRectanglePro(carRect, {20, 10}, rotation, RED);

  // Draw velocity vector (heading)
  DrawLineV(position, Vector2Add(position, Vector2Scale(velocity, 0.5f)), GREEN);
}

/**
 * @brief Adds a point to the list of waypoints the car should follow.
 *
 * @param point The new waypoint coordinates.
 */
void Car::addWaypoint(Vector2 point) { waypoints.push_back(point); }

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
void Car::seek(Vector2 target) {
  Vector2 desired = Vector2Subtract(target, position);
  desired = Vector2Normalize(desired);
  desired = Vector2Scale(desired, maxSpeed);

  Vector2 steer = Vector2Subtract(desired, velocity);

  // Limit the steering force to maxForce
  if (Vector2Length(steer) > maxForce) {
    steer = Vector2Scale(Vector2Normalize(steer), maxForce);
  }

  applyForce(steer);
}
