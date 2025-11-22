#include "entities/World.hpp"
#include "raylib.h"
#include <format>

World::World(int width, int height) : width(width), height(height), showGrid(true) {
  // World initialized with default grid enabled
}

void World::update(double /*dt*/) {
  // World logic if any
}

void World::draw() {
  // Draw World Bounds
  DrawRectangleLines(0, 0, width, height, BLACK);

  if (showGrid) {
    // Draw Grid
    for (int i = 0; i <= width; i += 200) {
      DrawLine(i, 0, i, height, Fade(LIGHTGRAY, 0.5f));
      DrawText(std::format("{}", i).c_str(), i + 5, 10, 20, GRAY);
    }
    for (int i = 0; i <= height; i += 200) {
      DrawLine(0, i, width, i, Fade(LIGHTGRAY, 0.5f));
      DrawText(std::format("{}", i).c_str(), 10, i + 5, 20, GRAY);
    }
  }
}
