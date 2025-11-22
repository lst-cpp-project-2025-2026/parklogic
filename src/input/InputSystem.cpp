#include "input/InputSystem.hpp"
#include "events/InputEvents.hpp"

InputSystem::InputSystem(std::shared_ptr<EventBus> bus, const Window &win) : eventBus(bus), window(win) {}

void InputSystem::update() {
  // Keyboard
  int key = GetKeyPressed();
  while (key != 0) {
    eventBus->publish(KeyPressedEvent{key});
    activeKeys.insert(key);
    key = GetKeyPressed();
  }

  // Check for released keys
  std::erase_if(activeKeys, [this](int k) {
    if (IsKeyUp(k)) {
      eventBus->publish(KeyReleasedEvent{k});
      return true;
    }
    return false;
  });

  // Mouse Position Math
  Vector2 rawPos = GetMousePosition();
  float scale = window.getScale();
  Vector2 offset = window.getOffset();

  // Convert Screen Space -> Logical Game Space (1280x720)
  Vector2 logPos = {(rawPos.x - offset.x) / scale, (rawPos.y - offset.y) / scale};

  // Always publish move (UI needs this for hover states)
  eventBus->publish(MouseMovedEvent{logPos});

  // Mouse Clicks
  // Use MOUSE_BUTTON_LEFT (Raylib 5.0+)
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    eventBus->publish(MouseClickEvent{MOUSE_BUTTON_LEFT, logPos, true});

  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    eventBus->publish(MouseClickEvent{MOUSE_BUTTON_LEFT, logPos, false});

  // Optional: Right click logic...
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    eventBus->publish(MouseClickEvent{MOUSE_BUTTON_RIGHT, logPos, true});

  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
    eventBus->publish(MouseClickEvent{MOUSE_BUTTON_RIGHT, logPos, false});
}
