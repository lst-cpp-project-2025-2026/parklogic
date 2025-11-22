#include "scenes/GameScene.hpp"
#include "config.hpp"
#include "core/Logger.hpp"
#include "entities/Car.hpp"
#include "entities/World.hpp"
#include "events/GameEvents.hpp"
#include "events/InputEvents.hpp"
#include "ui/DebugOverlay.hpp"
#include <format>

GameScene::GameScene(std::shared_ptr<EventBus> bus) : eventBus(bus) {}

GameScene::~GameScene() {
  // eventTokens vector will automatically destruct here,
  // triggering Subscription::~Subscription, removing listeners from Bus.
  Logger::Info("GameScene Destroyed - Listeners Unsubscribed");
}

void GameScene::load() {
  Logger::Info("Loading GameScene...");

  // Setup World with a large size
  world = std::make_unique<World>(3000, 3000);

  // Spawn multiple cars at random positions
  for (int i = 0; i < 10; ++i) {
    float x = GetRandomValue(50, world->getWidth() - 50);
    float y = GetRandomValue(50, world->getHeight() - 50);
    cars.push_back(std::make_unique<Car>(Vector2{x, y}, world.get()));
  }

  // Setup Camera
  camera.zoom = 0.5f;
  camera.target = {world->getWidth() / 2.0f, world->getHeight() / 2.0f};
  camera.offset = {Config::LOGICAL_WIDTH / 2.0f, Config::LOGICAL_HEIGHT / 2.0f};
  camera.rotation = 0.0f;

  // --- Subscribe to Events (Store tokens!) ---

  // 1. Handle Key Presses
  eventTokens.push_back(eventBus->subscribe<KeyPressedEvent>([this](const KeyPressedEvent &e) {
    keysDown.insert(e.key);
    // ESC: Menu
    if (e.key == KEY_ESCAPE) {
      Logger::Info("Switching to MainMenu");
      eventBus->publish(SceneChangeEvent{SceneType::MainMenu});
    }
    // P: Pause
    if (e.key == KEY_P) {
      isPaused = !isPaused;
      Logger::Info("Game Paused: {}", isPaused);
      if (isPaused) {
        eventBus->publish(GamePausedEvent{});
      } else {
        eventBus->publish(GameResumedEvent{});
      }
    }
    // F1: Debug
    if (e.key == KEY_F1) {
      if (debugOverlay) {
        debugOverlay->setActive(!debugOverlay->isActive());
      }
    }
  }));

  // 2. Handle Key Releases
  eventTokens.push_back(
      eventBus->subscribe<KeyReleasedEvent>([this](const KeyReleasedEvent &e) { keysDown.erase(e.key); }));

  // Setup UI
  debugOverlay = std::make_shared<DebugOverlay>(this, eventBus);
  ui.add(debugOverlay);
}

void GameScene::unload() {
  world.reset();
  cars.clear();
  // Explicitly clear tokens (optional, but good for immediate cleanup)
  eventTokens.clear();
}

void GameScene::handleInput(double dt) {
  // Camera Movement Speed (increases when zoomed out)
  float speed = 500.0f / camera.zoom;

  // Event-based input tracking
  if (keysDown.contains(KEY_W))
    camera.target.y -= speed * dt;
  if (keysDown.contains(KEY_S))
    camera.target.y += speed * dt;
  if (keysDown.contains(KEY_A))
    camera.target.x -= speed * dt;
  if (keysDown.contains(KEY_D))
    camera.target.x += speed * dt;

  // Zoom
  float wheel = GetMouseWheelMove();
  if (wheel != 0)
    camera.zoom += wheel * 0.1f;
  if (keysDown.contains(KEY_E))
    camera.zoom += 1.0f * dt;
  if (keysDown.contains(KEY_Q))
    camera.zoom -= 1.0f * dt;

  // Clamping
  if (camera.zoom < 0.1f)
    camera.zoom = 0.1f;
  if (camera.zoom > 3.0f)
    camera.zoom = 3.0f;

  if (camera.target.x < 0)
    camera.target.x = 0;
  if (camera.target.y < 0)
    camera.target.y = 0;
  if (camera.target.x > world->getWidth())
    camera.target.x = world->getWidth();
  if (camera.target.y > world->getHeight())
    camera.target.y = world->getHeight();
}

void GameScene::update(double dt) {
  handleInput(dt);
  ui.update(dt);

  if (!isPaused) {
    if (world)
      world->update(dt);
    for (auto &car : cars) {
      car->updateWithNeighbors(dt, &cars);
    }
  }
}

void GameScene::draw() {
  // --- World Render Pass ---
  BeginMode2D(camera);
  ClearBackground(RAYWHITE);

  if (world)
    world->draw();
  for (auto &car : cars) {
    car->draw();
  }
  EndMode2D();

  // --- UI Render Pass ---
  if (isPaused) {
    DrawText("PAUSED", Config::LOGICAL_WIDTH / 2 - 100, 50, 60, MAROON);
  }

  DrawText("P: Pause | F1: Debug | WASD: Move | Scroll: Zoom | ESC: Menu", 10, Config::LOGICAL_HEIGHT - 30, 20,
           DARKGRAY);

  ui.draw();
}
