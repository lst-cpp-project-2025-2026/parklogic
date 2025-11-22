#pragma once
#include <functional>
/**
 * @class GameLoop
 * @brief Manages the main game loop with a fixed timestep.
 *
 * The GameLoop class implements a fixed timestep game loop, ensuring consistent
 * game logic updates regardless of the rendering framerate.
 */
class GameLoop {
public:
  /**
   * @brief Runs the game loop.
   *
   * @param update Function to call for updating game logic (receives delta time).
   * @param render Function to call for rendering the frame.
   * @param running Function that returns true if the loop should continue.
   */
  void run(std::function<void(double)> update, std::function<void()> render, std::function<bool()> running);
};
