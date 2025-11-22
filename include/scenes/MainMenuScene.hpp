#pragma once
#include "scenes/IScene.hpp"
#include "ui/UIManager.hpp"
/**
 * @class MainMenuScene
 * @brief The main menu scene.
 *
 * Displays the game title and buttons to start the game or exit.
 */
class MainMenuScene : public IScene {
public:
  /**
   * @brief Constructs the MainMenuScene.
   *
   * @param bus Shared pointer to the EventBus.
   */
  explicit MainMenuScene(std::shared_ptr<EventBus> bus);

  /**
   * @brief Initializes the menu UI.
   */
  void load() override;

  /**
   * @brief Cleans up the menu resources.
   */
  void unload() override;

  /**
   * @brief Updates the menu UI.
   *
   * @param dt Delta time in seconds.
   */
  void update(double dt) override;

  /**
   * @brief Draws the menu background and UI.
   */
  void draw() override;

private:
  std::shared_ptr<EventBus> eventBus; ///< EventBus for communication.
  UIManager ui;                       ///< UI Manager for the menu.
};
