#pragma once
#include "core/EventBus.hpp" // Ensure Subscription class is visible
#include "events/GameEvents.hpp"
#include "scenes/IScene.hpp"
#include <memory>

/**
 * @class SceneManager
 * @brief Manages the active game scene and transitions.
 *
 * The SceneManager handles loading, unloading, updating, and rendering the current scene.
 * It also listens for SceneChangeEvents to switch scenes.
 */
class SceneManager {
public:
  /**
   * @brief Constructs the SceneManager.
   *
   * @param bus Shared pointer to the EventBus.
   */
  explicit SceneManager(std::shared_ptr<EventBus> bus);

  /**
   * @brief Destructor.
   */
  ~SceneManager() = default;

  /**
   * @brief Updates the current scene and handles pending transitions.
   *
   * @param dt Delta time in seconds.
   */
  void update(double dt);

  /**
   * @brief Renders the current scene.
   */
  void render();

  /**
   * @brief Sets the active scene.
   *
   * Unloads the current scene (if any) and loads the new one.
   *
   * @param type The type of scene to switch to.
   */
  void setScene(SceneType type);

private:
  std::shared_ptr<EventBus> eventBus;   ///< EventBus for communication.
  std::unique_ptr<IScene> currentScene; ///< The currently active scene.

  Subscription sceneChangeToken; ///< Token for scene change event subscription.

  bool changeQueued = false;                 ///< Flag indicating a scene change is pending.
  SceneType nextScene = SceneType::MainMenu; ///< The next scene to load.
};
