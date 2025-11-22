#include "ui/DebugOverlay.hpp"
#include "config.hpp"
#include "scenes/GameScene.hpp"
#include <format>

DebugOverlay::DebugOverlay(GameScene *scene, std::shared_ptr<EventBus> bus)
    : UIElement({10, 10}, {0, 0}, bus), scene(scene) {
  // Default to hidden, toggled by F1 in GameScene
  visible = false;
}

void DebugOverlay::update(double /*dt*/) {
  // No specific update logic needed for now
}

void DebugOverlay::draw() {
  if (!visible)
    return;

  int x = static_cast<int>(position.x);
  int y = static_cast<int>(position.y);
  int fontSize = 20;
  int dy = 25;

  struct DebugLine {
    std::string text;
    Color color;
  };
  std::vector<DebugLine> lines;

  lines.push_back(DebugLine{std::format("FPS: {}", GetFPS()), LIME});

  const auto &camera = scene->getCamera();
  lines.push_back(DebugLine{std::format("Zoom: {:.2f}", camera.zoom), WHITE});
  lines.push_back(DebugLine{std::format("Cam Target: {:.0f}, {:.0f}", camera.target.x, camera.target.y), WHITE});

  lines.push_back(DebugLine{std::format("Listeners: {}", scene->getListenerCount()), YELLOW});

  // Get Mouse Info
  Vector2 mouse = GetMousePosition();
  // Recalculate logical for display
  float scale =
      std::min((float)GetScreenWidth() / Config::LOGICAL_WIDTH, (float)GetScreenHeight() / Config::LOGICAL_HEIGHT);
  Vector2 offset = {(GetScreenWidth() - Config::LOGICAL_WIDTH * scale) * 0.5f,
                    (GetScreenHeight() - Config::LOGICAL_HEIGHT * scale) * 0.5f};
  Vector2 logical = {(mouse.x - offset.x) / scale, (mouse.y - offset.y) / scale};

  lines.push_back(DebugLine{std::format("Mouse Raw: {:.0f},{:.0f}", mouse.x, mouse.y), YELLOW});
  lines.push_back(DebugLine{std::format("Mouse Log: {:.0f},{:.0f}", logical.x, logical.y), YELLOW});

  // Calculate dimensions
  int maxWidth = 0;
  for (const auto &line : lines) {
    int width = MeasureText(line.text.c_str(), fontSize);
    if (width > maxWidth)
      maxWidth = width;
  }

  int totalHeight = lines.size() * dy;
  int paddingX = 10;
  int paddingY = 10;

  // Draw Background
  // Using -5 offset as in original code for top-left
  DrawRectangle(x - 5, y - 5, maxWidth + paddingX * 2, totalHeight + paddingY, Fade(BLACK, 0.8f));

  // Draw Text
  for (const auto &line : lines) {
    DrawText(line.text.c_str(), x, y, fontSize, line.color);
    y += dy;
  }
}
