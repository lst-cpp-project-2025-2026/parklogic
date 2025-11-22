#pragma once
#include "core/EventBus.hpp"
#include "core/EventLogger.hpp"
#include "core/GameLoop.hpp"
#include "core/Window.hpp"
#include "input/InputSystem.hpp"
#include "scenes/SceneManager.hpp"
#include <memory>

/**
 * @class Application
 * @brief The main entry point for the game engine.
 *
 * The Application class manages the core components of the engine, including
 * the window, event bus, input system, scene manager, and game loop. It coordinates
 * the initialization, updating, and rendering of the game.
 */
class Application {
public:
  /**
   * @brief Constructs the Application and initializes core systems.
   */
  Application();

  /**
   * @brief Destructor.
   *
   * Cleans up resources. The Subscription token automatically unsubscribes on destruction.
   */
  ~Application() = default;

  /**
   * @brief Starts the main game loop.
   *
   * This method blocks until the game is closed.
   */
  void run();

private:
  /**
   * @brief Updates the game state.
   *
   * @param dt Delta time in seconds since the last frame.
   */
  void update(double dt);

  /**
   * @brief Renders the current frame.
   */
  void render();

  std::shared_ptr<EventBus> eventBus;         ///< The central event bus for communication.
  std::unique_ptr<Window> window;             ///< The main game window.
  std::unique_ptr<GameLoop> gameLoop;         ///< The game loop manager.
  std::unique_ptr<InputSystem> inputSystem;   ///< The input handling system.
  std::unique_ptr<SceneManager> sceneManager; ///< The scene manager.

  bool isRunning = true; ///< Flag indicating if the application is running.

  Subscription closeEventToken; ///< Token for the window close event subscription.

  std::unique_ptr<EventLogger> eventLogger; ///< Logger for debugging events.
};
