#pragma once

/**
 * @class Entity
 * @brief Abstract base class for all game entities.
 *
 * Defines the interface for objects that can be updated and drawn in the game world.
 */
class Entity {
public:
  virtual ~Entity() = default;

  /**
   * @brief Updates the entity state.
   *
   * @param dt Delta time in seconds.
   */
  virtual void update(double dt) = 0;

  /**
   * @brief Draws the entity.
   */
  virtual void draw() = 0;
};
