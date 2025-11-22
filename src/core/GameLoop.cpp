#include "core/GameLoop.hpp"
#include "config.hpp"
#include "raylib.h"

void GameLoop::run(std::function<void(double)> update, std::function<void()> render, std::function<bool()> running) {

  const double dt = Config::FIXED_DELTA_TIME;
  double currentTime = GetTime();
  double accumulator = 0.0;

  while (running()) {
    double newTime = GetTime();
    double frameTime = newTime - currentTime;
    currentTime = newTime;

    // Cap frame time to avoid spiral of death
    if (frameTime > 0.25)
      frameTime = 0.25;
    accumulator += frameTime;

    // Fixed timestep update
    while (accumulator >= dt) {
      update(dt);
      accumulator -= dt;
    }
    render();
  }
}
