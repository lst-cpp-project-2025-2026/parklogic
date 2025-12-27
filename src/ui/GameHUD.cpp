#include "ui/GameHUD.hpp"
#include "config.hpp"
#include "events/GameEvents.hpp"
#include "raylib.h"
#include "ui/DashboardOverlay.hpp"
#include "ui/UIButton.hpp"
#include <format>

/**
 * @file GameHUD.cpp
 * @brief Implementation of GameHUD.
 */

GameHUD::GameHUD(std::shared_ptr<EventBus> bus, EntityManager *entityManager) : eventBus(bus) {
  // Setup UI Elements
  uiManager.add(std::make_shared<DashboardOverlay>(eventBus, entityManager));

  auto spawnBtn = std::make_shared<UIButton>(Vector2{10, 10}, Vector2{150, 40}, "Spawn Car", eventBus);
  spawnBtn->setOnClick([this]() { eventBus->publish(SpawnCarRequestEvent{}); });
  uiManager.add(spawnBtn);

  auto speedBtn = std::make_shared<UIButton>(Vector2{10, 60}, Vector2{150, 40}, "Speed: 1.0x", eventBus);
  std::weak_ptr<UIButton> weakSpeedBtn = speedBtn;
  speedBtn->setOnClick([this, weakSpeedBtn]() {
    currentSpeed += 0.5;
    if (currentSpeed > 5.0) {
      currentSpeed = 1.0;
    }
    eventBus->publish(SimulationSpeedChangedEvent{currentSpeed});
    // Update button text
    if (auto btn = weakSpeedBtn.lock()) {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Speed: %.1fx", currentSpeed);
      btn->setText(buffer);
    }
  });
  uiManager.add(speedBtn);

  // Auto Spawn Button
  auto autoSpawnBtn = std::make_shared<UIButton>(Vector2{10, 110}, Vector2{150, 40}, "Auto: Off", eventBus);
  autoSpawnBtn->setOnClick([this]() { eventBus->publish(CycleAutoSpawnLevelEvent{}); });
  uiManager.add(autoSpawnBtn);

  // Subscribe to Auto Spawn Level Changes to update text
  eventTokens.push_back(
      eventBus->subscribe<AutoSpawnLevelChangedEvent>([autoSpawnBtn](const AutoSpawnLevelChangedEvent &e) {
        std::string text = (e.newLevel == 0) ? "Auto: Off" : std::format("Auto: Lvl {}", e.newLevel);
        autoSpawnBtn->setText(text);
      }));

  // Subscribe to Pause Events to toggle pause text visibility
  eventTokens.push_back(eventBus->subscribe<GamePausedEvent>([this](const GamePausedEvent &) { isPaused = true; }));

  eventTokens.push_back(eventBus->subscribe<GameResumedEvent>([this](const GameResumedEvent &) { isPaused = false; }));
}

GameHUD::~GameHUD() { eventTokens.clear(); }

void GameHUD::update(double dt) { uiManager.update(dt); }

void GameHUD::draw() {
  uiManager.draw();

  // Draw Static HUD Text
  if (isPaused) {
    DrawText("PAUSED", Config::LOGICAL_WIDTH / 2 - 100, 50, 60, MAROON);
  }

  DrawText("WASD: Move | Scroll: Zoom | ESC: Menu", 10, Config::LOGICAL_HEIGHT - 30, 20, DARKGRAY);
}
