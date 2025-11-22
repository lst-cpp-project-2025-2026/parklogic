#pragma once
#include "core/EventBus.hpp" // Need Subscription definition
#include "entities/Car.hpp"
#include "entities/World.hpp"
#include "scenes/IScene.hpp"
#include "ui/UIManager.hpp"
#include <memory>
#include <unordered_set>
#include <vector>

/**
 * @class GameScene
 * @brief The main gameplay scene.
 *
 * Manages the game world, entities (cars), camera, and UI.
 * Handles input for camera movement and game control (pause, debug).
 */
class GameScene : public IScene {
public:
  /**
   * @brief Constructs the GameScene.
   *
   * @param bus Shared pointer to the EventBus.
   */
  explicit GameScene(std::shared_ptr<EventBus> bus);

  /**
   * @brief Destructor.
   *
   * Ensures all event subscriptions are cleaned up.
   */
  ~GameScene() override;

  /**
   * @brief Initializes the scene.
   *
   * Creates the world, spawns cars, sets up the camera, and subscribes to events.
   */
  void load() override;

  /**
   * @brief Cleans up the scene.
   *
   * Resets the world and clears entities.
   */
  void unload() override;

  /**
   * @brief Updates the game logic.
   *
   * @param dt Delta time in seconds.
   */
  void update(double dt) override;

  /**
   * @brief Renders the game world and UI.
   */
  void draw() override;

  // Getters for DebugOverlay
  const Camera2D &getCamera() const { return camera; }
  const World *getWorld() const { return world.get(); }
  size_t getListenerCount() const { return eventTokens.size(); }

private:
  /**
   * @brief Handles user input for camera control.
   *
   * @param dt Delta time in seconds.
   */
  void handleInput(double dt);

  std::shared_ptr<EventBus> eventBus; ///< EventBus for communication.

  // Store subscriptions to keep them alive while the scene is active
  std::vector<Subscription> eventTokens;

  std::unique_ptr<World> world;           ///< The game world.
  std::vector<std::unique_ptr<Car>> cars; ///< List of car entities.

  // FIX: C++20 zero-initialization for struct
  Camera2D camera = {}; ///< The game camera.

  UIManager ui;                                     ///< UI Manager for the scene.
  std::shared_ptr<class DebugOverlay> debugOverlay; ///< Debug overlay UI element.

  bool isPaused = false;            ///< Pause state flag.
  std::unordered_set<int> keysDown; ///< Set of currently pressed keys.
};
