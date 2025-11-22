#pragma once
#include "ui/UIElement.hpp"
#include <vector>

/**
 * @class UIManager
 * @brief Manages a collection of UI elements.
 *
 * Handles updating and drawing multiple UI elements.
 */
class UIManager {
public:
  /**
   * @brief Adds a UI element to the manager.
   * @param el Shared pointer to the UIElement.
   */
  void add(std::shared_ptr<UIElement> el) { elements.push_back(el); }

  /**
   * @brief Updates all active UI elements.
   * @param dt Delta time in seconds.
   */
  void update(double dt) {
    for (auto &e : elements)
      if (e->isActive())
        e->update(dt);
  }

  /**
   * @brief Draws all active UI elements.
   */
  void draw() {
    for (auto &e : elements)
      if (e->isActive())
        e->draw();
  }

private:
  std::vector<std::shared_ptr<UIElement>> elements; ///< List of managed UI elements.
};
