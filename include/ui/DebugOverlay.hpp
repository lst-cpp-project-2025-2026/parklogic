#pragma once
#include "ui/UIElement.hpp"

class GameScene; // Forward declaration

/**
 * @class DebugOverlay
 * @brief Displays debug information on the screen.
 *
 * Shows FPS, camera details, car status, and input info.
 */
class DebugOverlay : public UIElement {
public:
  /**
   * @brief Constructs the DebugOverlay.
   *
   * @param scene Pointer to the GameScene to access game state.
   * @param bus Shared pointer to the EventBus.
   */
  DebugOverlay(GameScene *scene, std::shared_ptr<EventBus> bus);

  void update(double dt) override;
  void draw() override;

private:
  GameScene *scene; ///< Pointer to the game scene.
};
