#include "core/Window.hpp"
#include "config.hpp"
#include "core/Logger.hpp"
#include "events/WindowEvents.hpp"
#include <algorithm>
#include <stdexcept>

Window::Window(std::shared_ptr<EventBus> bus) : eventBus(bus), scale(1.0f), offsetX(0.0f), offsetY(0.0f) {

  initRaylib();

  // Initialize the render texture for the logical resolution
  target = LoadRenderTexture(Config::LOGICAL_WIDTH, Config::LOGICAL_HEIGHT);
  if (!IsRenderTextureValid(target)) {
    throw std::runtime_error("Failed to load Render Texture");
  }
  SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

  currentWidth = GetScreenWidth();
  currentHeight = GetScreenHeight();
  updateDimensions();

  Logger::Info("Window Initialized: {}x{}", currentWidth, currentHeight);
}

Window::~Window() {
  UnloadRenderTexture(target);
  CloseWindow();
  Logger::Info("Window Closed");
}

void Window::initRaylib() {
  if (Config::VSYNC_ENABLED)
    SetConfigFlags(FLAG_VSYNC_HINT);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetTraceLogLevel(LOG_WARNING);

  InitWindow(Config::INITIAL_WINDOW_WIDTH, Config::INITIAL_WINDOW_HEIGHT, Config::WINDOW_TITLE);
  SetExitKey(KEY_NULL);
  SetWindowMinSize(640, 360);
  SetTargetFPS(Config::TARGET_FPS);

  if (!IsWindowReady())
    throw std::runtime_error("Failed to Initialize Raylib Window");
}

bool Window::shouldClose() const { return WindowShouldClose(); }

void Window::updateDimensions() {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  scale = std::min((float)screenW / Config::LOGICAL_WIDTH, (float)screenH / Config::LOGICAL_HEIGHT);
  offsetX = (screenW - (Config::LOGICAL_WIDTH * scale)) * 0.5f;
  offsetY = (screenH - (Config::LOGICAL_HEIGHT * scale)) * 0.5f;

  if (screenW != currentWidth || screenH != currentHeight) {
    currentWidth = screenW;
    currentHeight = screenH;
    eventBus->publish(WindowResizeEvent{currentWidth, currentHeight});
  }
}

void Window::beginDrawing() {
  updateDimensions();
  BeginTextureMode(target);
  ClearBackground(BLACK);
}

void Window::endDrawing() {
  EndTextureMode();
  BeginDrawing();
  ClearBackground(BLACK);

  Rectangle source = {0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height};
  Rectangle dest = {offsetX, offsetY, (float)Config::LOGICAL_WIDTH * scale, (float)Config::LOGICAL_HEIGHT * scale};
  DrawTexturePro(target.texture, source, dest, {0, 0}, 0.0f, WHITE);

  EndDrawing();
}
