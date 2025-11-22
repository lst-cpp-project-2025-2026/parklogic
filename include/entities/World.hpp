#pragma once
#include "entities/Entity.hpp"

/**
 * @class World
 * @brief Represents the game world boundaries and grid.
 *
 * The World class manages the playable area, rendering the boundary lines and
 * an optional grid for visual reference.
 */
class World : public Entity {
public:
  /**
   * @brief Constructs the World with specified dimensions.
   *
   * @param width Width of the world in pixels.
   * @param height Height of the world in pixels.
   */
  World(int width, int height);

  /**
   * @brief Updates the world state.
   *
   * @param dt Delta time in seconds.
   */
  void update(double dt) override;

  /**
   * @brief Draws the world boundaries and grid.
   */
  void draw() override;

  /**
   * @brief Sets the visibility of the grid.
   * @param enabled True to show the grid, false to hide it.
   */
  void setGridEnabled(bool enabled) { showGrid = enabled; }

  /**
   * @brief Checks if the grid is currently enabled.
   * @return True if the grid is visible.
   */
  bool isGridEnabled() const { return showGrid; }

  /**
   * @brief Toggles the visibility of the grid.
   */
  void toggleGrid() { showGrid = !showGrid; }

  int getWidth() const { return width; }
  int getHeight() const { return height; }

private:
  int width;     ///< World width.
  int height;    ///< World height.
  bool showGrid; ///< Flag for grid visibility.
};
