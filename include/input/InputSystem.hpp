#pragma once
#include "core/EventBus.hpp"
#include "core/Window.hpp"
#include <unordered_set>

class InputSystem {
public:
  InputSystem(std::shared_ptr<EventBus> bus, const Window &win);
  void update();

private:
  std::shared_ptr<EventBus> eventBus;
  const Window &window;
  std::unordered_set<int> activeKeys;
};
