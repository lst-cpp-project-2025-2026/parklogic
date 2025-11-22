#pragma once
#include "ui/UIElement.hpp"
#include <functional>
#include <string>

/**
 * @class UIButton
 * @brief A clickable button UI element.
 *
 * Handles mouse hover and click events to trigger a callback.
 */
class UIButton : public UIElement {
public:
  /**
   * @brief Constructs a UIButton.
   *
   * @param pos Position of the button.
   * @param size Size of the button.
   * @param text Text displayed on the button.
   * @param bus Shared pointer to the EventBus.
   */
  UIButton(Vector2 pos, Vector2 size, const std::string &text, std::shared_ptr<EventBus> bus);

  void update(double dt) override;
  void draw() override;

  /**
   * @brief Sets the callback function to execute on click.
   * @param cb The callback function.
   */
  void setOnClick(std::function<void()> cb);

private:
  std::string text;              ///< Button label text.
  std::function<void()> onClick; ///< Click callback.
  bool isHovered = false;        ///< Hover state.
  bool isPressed = false;        ///< Pressed state.
};
