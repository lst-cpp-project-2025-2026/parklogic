#pragma once
#include "entities/Entity.hpp"
#include "raylib.h"
#include <deque>
#include <memory>
#include <vector>
#include <string>

class World;

/**
 * @class Car
 * @brief Represents an autonomous car entity.
 *
 * The Car class implements steering behaviors (seek) to navigate through waypoints.
 * It supports collision avoidance and dynamic waypoint generation.
 */
#include "entities/map/Waypoint.hpp"

class Car : public Entity {
public:
  /**
   * @brief Constructs a Car entity.
   *
   * @param startPos Initial position.
   * @param world Pointer to the game world for bounds checking.
   */
  Car(Vector2 startPos, const class World *world, Vector2 initialVelocity = {0, 0});

  /**
   * @brief Updates the car's physics and logic.
   *
   * @param dt Delta time in seconds.
   */
  void update(double dt) override;

  /**
   * @brief Updates the car with awareness of other cars.
   *
   * @param dt Delta time in seconds.
   * @param cars Pointer to the list of other cars for collision avoidance.
   */
  void updateWithNeighbors(double dt, const std::vector<std::unique_ptr<Car>> *cars = nullptr);

  /**
   * @brief Draws the car and its debug info (waypoints, velocity).
   */
  void draw() override;

  /**
   * @brief Adds a waypoint to the car's path.
   *
   * @param wp The target waypoint.
   */
  void addWaypoint(Waypoint wp);

  /**
   * @brief Sets the entire path of waypoints.
   * 
   * @param path Vector of waypoints.
   */
  void setPath(const std::vector<Waypoint>& path);

  /**
   * @brief Clears all waypoints.
   */
  void clearWaypoints();

  Vector2 getPosition() const { return position; }
  Vector2 getVelocity() const { return velocity; }

private:
  Vector2 position;
  Vector2 velocity;
  Vector2 acceleration;
  const World *world;

  float maxSpeed;
  float maxForce;

  std::deque<Waypoint> waypoints;

  /**
   * @brief Applies a force to the car's acceleration.
   *
   * @param force The force vector.
   */
  void applyForce(Vector2 force);

  /**
   * @brief Calculates and applies a steering force towards a target.
   *
   * @param wp The target waypoint.
   */
  void seek(const Waypoint& wp);
  std::string textureName;
};
