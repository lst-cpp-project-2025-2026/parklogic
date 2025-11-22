#include "ui/UIButton.hpp"
#include "events/InputEvents.hpp"

UIButton::UIButton(Vector2 pos, Vector2 sz, const std::string &txt, std::shared_ptr<EventBus> bus)
    : UIElement(pos, sz, bus), text(txt) {

  // 1. Subscribe to Mouse Move (Keep token!)
  // Updates hover state based on mouse position
  tokens.push_back(eventBus->subscribe<MouseMovedEvent>([this](const MouseMovedEvent &e) {
    if (!visible)
      return;
    Rectangle rect = {position.x, position.y, size.x, size.y};
    isHovered = CheckCollisionPointRec(e.position, rect);
  }));

  // 2. Subscribe to Mouse Click (Keep token!)
  // Handles click logic: press down, release inside to trigger
  tokens.push_back(eventBus->subscribe<MouseClickEvent>([this](const MouseClickEvent &e) {
    if (!visible)
      return;

    // Only interact if hovered
    if (isHovered) {
      if (e.button == MOUSE_BUTTON_LEFT) {
        if (e.down) {
          isPressed = true;
        } else {
          // Mouse Release
          if (isPressed && onClick) {
            onClick();
          }
          isPressed = false;
        }
      }
    } else {
      // If we moved off the button and released, reset state
      if (!e.down)
        isPressed = false;
    }
  }));
}

void UIButton::setOnClick(std::function<void()> cb) { onClick = cb; }
void UIButton::update(double) {}

void UIButton::draw() {
  if (!visible)
    return;

  // Visual feedback
  Color c = isPressed ? DARKGRAY : (isHovered ? LIGHTGRAY : GRAY);

  DrawRectangleV(position, size, c);
  DrawRectangleLines(position.x, position.y, size.x, size.y, BLACK);

  int fontSize = 20;
  int txtW = MeasureText(text.c_str(), fontSize);
  DrawText(text.c_str(), (int)(position.x + (size.x - txtW) / 2), (int)(position.y + (size.y - fontSize) / 2), fontSize,
           BLACK);
}
