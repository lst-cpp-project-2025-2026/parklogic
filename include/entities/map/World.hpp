#pragma once
#include "entities/Entity.hpp"
#include <vector>
#include <string>

/**
 * @class World
 * @brief Represents the game world boundaries and grid.
 *
 * The World class manages the playable area, rendering the boundary lines and
 * an optional grid for visual reference.
 */

class World : public Entity {
public:
  World(float width, float height);

  void update(double dt) override;
  void draw() override;
  void drawOverlay(); // Draws grid and borders on top of entities

  void setGridEnabled(bool enabled) { showGrid = enabled; }
  bool isGridEnabled() const { return showGrid; }
  void toggleGrid() { showGrid = !showGrid; }

  void drawMask(); // Draws the dark foreground mask outside the world

  float getWidth() const { return width; }
  float getHeight() const { return height; }

private:
  float width;
  float height;
  bool showGrid;

  // Background
  std::vector<std::vector<int>> backgroundTiles; // Stores index of texture to use
  std::vector<std::string> tileTextures;         // Texture names
  float tileWidthMeter;
  float tileHeightMeter;
};
