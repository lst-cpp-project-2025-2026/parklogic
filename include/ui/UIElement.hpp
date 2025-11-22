#pragma once
#include "core/EventBus.hpp"
#include "raylib.h"
#include <memory>
#include <vector>

/**
 * @class UIElement
 * @brief Abstract base class for UI components.
 *
 * Defines the interface for UI elements that can be updated, drawn, and toggled.
 */
class UIElement {
public:
  /**
   * @brief Constructs a UIElement.
   *
   * @param pos Position of the element.
   * @param sz Size of the element.
   * @param bus Shared pointer to the EventBus.
   */
  UIElement(Vector2 pos, Vector2 sz, std::shared_ptr<EventBus> bus)
      : position(pos), size(sz), eventBus(bus), visible(true) {}

  virtual ~UIElement() = default; // Tokens vector clears automatically here

  /**
   * @brief Updates the UI element logic.
   *
   * @param dt Delta time in seconds.
   */
  virtual void update(double dt) = 0;

  /**
   * @brief Draws the UI element.
   */
  virtual void draw() = 0;

  /**
   * @brief Checks if the element is visible/active.
   * @return True if visible.
   */
  bool isActive() const { return visible; }

  /**
   * @brief Sets the visibility/active state of the element.
   * @param active True to make visible.
   */
  void setActive(bool active) { visible = active; }

protected:
  Vector2 position;                   ///< Position of the element.
  Vector2 size;                       ///< Size of the element.
  std::shared_ptr<EventBus> eventBus; ///< EventBus for communication.
  bool visible;                       ///< Visibility flag.

  // FIX: Store subscriptions here to keep them alive
  std::vector<Subscription> tokens; ///< List of event subscriptions.
};
