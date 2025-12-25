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
  enum class CarType { COMBUSTION, ELECTRIC };

  /**
   * @brief Constructs a Car entity.
   *
   * @param startPos Initial position.
   * @param world Pointer to the game world for bounds checking.
   * @param type The type of car (Combustion or Electric).
   */
  Car(Vector2 startPos, const class World *world, Vector2 initialVelocity, CarType type);

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
  void setParkingContext(const Module* fac, const Spot& spot, int spotIndex);
  const Module* getParkedFacility() const { return parkedFacility; }
  const Spot& getParkedSpot() const { return parkedSpot; }
  int getParkedSpotIndex() const { return parkedSpotIndex; }

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
  int parkedSpotIndex = -1; 

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
  /**
   * @brief Calculates and applies a steering force towards a target.
   *
   * @param wp The target waypoint.
   */
  void seek(const Waypoint& wp);
  std::string textureName;

  // New Members for Traffic Overhaul
public:
  enum class Priority { PRIORITY_PRICE, PRIORITY_DISTANCE }; // Renamed from PRICE/DISTANCE to avoid potential macros? No, just good practice.
  
  Priority getPriority() const { return priority; }
  void setPriority(Priority p) { priority = p; }
  
  bool getEnteredFromLeft() const { return enteredFromLeft; }
  void setEnteredFromLeft(bool left) { enteredFromLeft = left; }
  
  CarType getType() const { return type; }
  
  void charge(float amount);
  float getBatteryLevel() const { return batteryLevel; }
  
  void setParkingDuration(float duration) { parkingDuration = duration; }

private:
  CarType type;
  Priority priority = Priority::PRIORITY_DISTANCE; // Default
  bool enteredFromLeft = true; // Default
  float batteryLevel = 100.0f; // 0-100%
  float parkingDuration = 0.0f; // Assigned when parking starts
};
