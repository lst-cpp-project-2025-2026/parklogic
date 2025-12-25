#include "scenes/GameScene.hpp"
#include "config.hpp"
#include "core/EntityManager.hpp"
#include "core/Logger.hpp"
#include "events/GameEvents.hpp"
#include "events/InputEvents.hpp"
#include "raymath.h"
#include "systems/CameraSystem.hpp"
#include "systems/TrafficSystem.hpp"
#include "ui/GameHUD.hpp"
#include <format>

GameScene::GameScene(std::shared_ptr<EventBus> bus, MapConfig config) : eventBus(bus), config(config) {}

GameScene::~GameScene() { Logger::Info("GameScene Destroyed"); }

void GameScene::load() {
  Logger::Info("Loading GameScene (Generated World)...");

  // Initialize Managers
  cameraSystem = std::make_unique<CameraSystem>(eventBus);
  entityManager = std::make_unique<EntityManager>(eventBus);
  trafficSystem = std::make_unique<TrafficSystem>(eventBus, *entityManager);
  gameHUD = std::make_unique<GameHUD>(eventBus, entityManager.get());

  // Generate World via Event
  eventBus->publish(GenerateWorldEvent{config});

  // Setup Camera
  // Setup Camera
  cameraSystem->setZoom(1.0f);

  // Camera setup is now handled via WorldBoundsEvent in CameraSystem

  // Subscribe to Events
  eventTokens.push_back(eventBus->subscribe<KeyPressedEvent>([this](const KeyPressedEvent &e) {
    keysDown.insert(e.key);
    if (e.key == KEY_ESCAPE) {
      Logger::Info("Switching to MainMenu");
      eventBus->publish(SceneChangeEvent{SceneType::MainMenu, {}});
    }
    if (e.key == KEY_P) {
      if (isPaused) {
        eventBus->publish(GameResumedEvent{});
      } else {
        eventBus->publish(GamePausedEvent{});
      }
    }
  }));

  eventTokens.push_back(
      eventBus->subscribe<KeyReleasedEvent>([this](const KeyReleasedEvent &e) { keysDown.erase(e.key); }));

  eventTokens.push_back(eventBus->subscribe<GamePausedEvent>([this](const GamePausedEvent &) { isPaused = true; }));
  eventTokens.push_back(eventBus->subscribe<GameResumedEvent>([this](const GameResumedEvent &) { isPaused = false; }));

  // Mouse Click Handling
  eventTokens.push_back(eventBus->subscribe<MouseClickEvent>([this](const MouseClickEvent &e) {
    if (e.down && e.button == MOUSE_BUTTON_LEFT) {
      // Construct the Render Camera with PPM scaling (same as CameraSystem uses for drawing)
      Camera2D renderCamera = cameraSystem->getCamera();
      renderCamera.zoom *= Config::PPM;

      // e.position is already in Logical Coordinates (thanks to InputSystem)
      Vector2 worldPos = GetScreenToWorld2D(e.position, renderCamera);

      EntitySelectedEvent selectionEvent;
      selectionEvent.type = SelectionType::GENERAL; // Default

      bool found = false;

      // 1. Check Cars
      if (entityManager) {
        for (const auto &car : entityManager->getCars()) {
          Vector2 pos = car->getPosition();
          // Check distance in World Space (Meters)
          // Car radius ~0.5m - 1.0m?
          // Using 0.8m as clickable radius
          if (CheckCollisionPointCircle(worldPos, pos, 0.8f)) {
            selectionEvent.type = SelectionType::CAR;
            selectionEvent.car = car.get();
            found = true;
            break;
          }
        }

        // 2. Check Facilities
        if (!found) {
          for (const auto &mod : entityManager->getModules()) {
            Rectangle rec = {mod->worldPosition.x, mod->worldPosition.y, mod->getWidth(), mod->getHeight()};

            if (CheckCollisionPointRec(worldPos, rec)) {
              selectionEvent.type = SelectionType::FACILITY;
              selectionEvent.module = mod.get();
              found = true;

              // 3. Check Spots logic
              int bestSpotIndex = -1;
              float minDistSq = 1000000.0f;
              // TUNING: Spot click radius in meters
              float threshold = 4.0f;
              float thresholdSq = threshold * threshold;

              for (size_t i = 0; i < mod->getSpotCount(); i++) {
                Spot s = mod->getSpot(i);
                Vector2 spotWorldPos = {mod->worldPosition.x + s.localPosition.x,
                                        mod->worldPosition.y + s.localPosition.y};

                float dSq = Vector2DistanceSqr(worldPos, spotWorldPos);
                if (dSq < thresholdSq && dSq < minDistSq) {
                  minDistSq = dSq;
                  bestSpotIndex = (int)i;
                }
              }

              if (bestSpotIndex != -1) {
                selectionEvent.type = SelectionType::SPOT;
                selectionEvent.module = mod.get();
                selectionEvent.spotIndex = bestSpotIndex;
              }

              break;
            }
          }
        }
      }

      eventBus->publish(selectionEvent);
    }
  }));

  // Camera Zoom is now handled by CameraSystem
}

void GameScene::unload() {
  entityManager->clear();
  eventTokens.clear();
}

void GameScene::handleInput() {
  // Camera movement is now handled by CameraSystem

  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    eventBus->publish(CameraZoomEvent{wheel * 0.1f});
  }
}

void GameScene::update(double dt) {
  gameHUD->update(dt);

  if (!isPaused) {
    eventBus->publish(GameUpdateEvent{dt});
  }
}

void GameScene::draw() {
  handleInput();
  
  // Create a render camera that applies the PPM scaling
  eventBus->publish(BeginCameraEvent{});
  ClearBackground(RAYWHITE);

  eventBus->publish(DrawWorldEvent{});

  eventBus->publish(EndCameraEvent{});

  eventBus->publish(EndCameraEvent{});

  gameHUD->draw();
}
