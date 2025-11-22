#pragma once
#include "core/EventBus.hpp"
#include "raylib.h"
#include <memory>

/**
 * @class Window
 * @brief Manages the game window and rendering context using Raylib.
 *
 * The Window class handles window creation, resizing, and scaling to maintain
 * a consistent logical resolution independent of the actual window size.
 */
class Window {
public:
  /**
   * @brief Constructs the Window.
   *
   * @param bus Shared pointer to the EventBus for publishing window events.
   */
  explicit Window(std::shared_ptr<EventBus> bus);

  /**
   * @brief Destructor.
   *
   * Closes the window and unloads resources.
   */
  ~Window();

  /**
   * @brief Checks if the window should close.
   *
   * @return True if the close button was pressed or the close key was triggered.
   */
  bool shouldClose() const;

  /**
   * @brief Prepares the window for drawing.
   *
   * Starts the texture mode for the render target and clears the background.
   */
  void beginDrawing();

  /**
   * @brief Ends the drawing frame.
   *
   * Ends texture mode, draws the render target to the screen with proper scaling,
   * and swaps buffers.
   */
  void endDrawing();

  /**
   * @brief Gets the current scale factor.
   *
   * @return The scale factor applied to the logical resolution.
   */
  float getScale() const { return scale; }

  /**
   * @brief Gets the offset for centering the content.
   *
   * @return The (x, y) offset vector.
   */
  Vector2 getOffset() const { return {offsetX, offsetY}; }

private:
  /**
   * @brief Initializes the Raylib window with configuration settings.
   */
  void initRaylib();

  /**
   * @brief Updates the window dimensions and scale based on current window size.
   *
   * Publishes a WindowResizeEvent if the size changes.
   */
  void updateDimensions();

  std::shared_ptr<EventBus> eventBus; ///< Event bus for publishing events.
  RenderTexture2D target;             ///< Render target for logical resolution drawing.
  float scale;                        ///< Current scale factor.
  float offsetX, offsetY;             ///< Offsets for centering.
  int currentWidth, currentHeight;    ///< Current window dimensions.
};
