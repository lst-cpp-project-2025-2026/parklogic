#include "scenes/MainMenuScene.hpp"
#include "config.hpp"
#include "events/GameEvents.hpp"
#include "events/WindowEvents.hpp"
#include "ui/UIButton.hpp"

MainMenuScene::MainMenuScene(std::shared_ptr<EventBus> bus) : eventBus(bus) {}

void MainMenuScene::load() {
  float cx = Config::LOGICAL_WIDTH / 2.0f;
  float cy = Config::LOGICAL_HEIGHT / 2.0f;
  float btnWidth = 200.0f;
  float btnHeight = 50.0f;
  float spacing = 20.0f;

  // Create "START" button
  auto start =
      std::make_shared<UIButton>(Vector2{cx - btnWidth / 2, cy}, Vector2{btnWidth, btnHeight}, "START", eventBus);
  start->setOnClick([this]() { eventBus->publish(SceneChangeEvent{SceneType::Game}); });

  // Create "EXIT" button
  auto exit = std::make_shared<UIButton>(Vector2{cx - btnWidth / 2, cy + btnHeight + spacing},
                                         Vector2{btnWidth, btnHeight}, "EXIT", eventBus);
  exit->setOnClick([this]() { eventBus->publish(WindowCloseEvent{}); });

  ui.add(start);
  ui.add(exit);
}
void MainMenuScene::unload() {}
void MainMenuScene::update(double dt) { ui.update(dt); }
void MainMenuScene::draw() {
  ClearBackground(GetColor(0x181818FF)); // Dark background

  const char *title = "ParkLogic";
  int fontSize = 60;
  int titleW = MeasureText(title, fontSize);
  float cx = Config::LOGICAL_WIDTH / 2.0f;

  DrawText(title, (int)(cx - (float)titleW / 2), 100, fontSize, RAYWHITE);
  ui.draw();
}
