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
#include "entities/map/Modules.hpp"

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
   * @brief Updates the car's state with awareness of other cars.
   *
   * @param dt Delta time in seconds.
   * @param cars Pointer to the list of other cars for collision avoidance.
   */
  void updateWithNeighbors(double dt, const std::vector<std::unique_ptr<Car>> *cars = nullptr);

  /**
   * @brief Draws the car and its debug info (waypoints, velocity).
   */
  void draw() override;

  // --- State Management ---
  enum class CarState {
      DRIVING,
      ALIGNING,
      PARKED,
      EXITING
  };

  CarState getState() const { return state; }
  void setState(CarState newState) { state = newState; }

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
  void setVelocity(Vector2 v) { velocity = v; }

  bool isReadyToLeave() const { return state == CarState::PARKED && parkingTimer <= 0.0f; }
  
  bool hasArrived() const { return waypoints.empty(); }

  // Context for Parking
  // Used to generate the exit path.
  void setParkingContext(const Module* fac, const Spot& spot);
  const Module* getParkedFacility() const { return parkedFacility; }
  const Spot& getParkedSpot() const { return parkedSpot; }

private:
  Vector2 position;
  Vector2 velocity;
  Vector2 acceleration;
  const World *world;
  
  CarState state = CarState::DRIVING;
  float parkingTimer = 0.0f;
  float targetRotation = 0.0f; 
  float currentRotation = 0.0f; // degrees, for smooth rendering
  
  const Module* parkedFacility = nullptr;
  Spot parkedSpot = {{0,0}, 0.0f, -1}; 

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
