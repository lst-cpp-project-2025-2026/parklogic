#pragma once
/**
 * @class IScene
 * @brief Interface for game scenes.
 *
 * Defines the lifecycle methods that all scenes must implement: load, unload, update, and draw.
 */
class IScene {
public:
  virtual ~IScene() = default;

  /**
   * @brief Called when the scene is loaded.
   *
   * Use this to initialize resources specific to the scene.
   */
  virtual void load() = 0;

  /**
   * @brief Called when the scene is unloaded.
   *
   * Use this to clean up resources specific to the scene.
   */
  virtual void unload() = 0;

  /**
   * @brief Updates the scene logic.
   *
   * @param dt Delta time in seconds.
   */
  virtual void update(double dt) = 0;

  /**
   * @brief Draws the scene content.
   */
  virtual void draw() = 0;
};
