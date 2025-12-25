#pragma once
#include "entities/map/Waypoint.hpp"
#include "raylib.h"
#include <vector>

struct MapConfig {
  int smallParkingCount = 1;
  int largeParkingCount = 1;
  int smallChargingCount = 1;
  int largeChargingCount = 0;
};

enum class SceneType { MainMenu, MapConfig, Game };
struct SceneChangeEvent {
  SceneType newScene;
  MapConfig config;
};

struct GenerateWorldEvent {
  MapConfig config;
};

struct WorldBoundsEvent {
  float width;
  float height;
};

struct GameUpdateEvent {
  double dt;
};

struct BeginCameraEvent {};
struct EndCameraEvent {};
struct DrawWorldEvent {};

struct GamePausedEvent {};
struct GameResumedEvent {};
struct ToggleDebugOverlayEvent {
  bool enabled;
};

struct CameraZoomEvent {
  float zoomDelta;
};

struct CameraMoveEvent {
  Vector2 delta;
};

struct SpawnCarEvent {};

struct SpawnCarRequestEvent {};

struct CreateCarEvent {
  Vector2 position;
  Vector2 velocity; // Initial velocity (sets heading)
  int carType; // 0: Combustion, 1: Electric
  int priority; // 0: Price, 1: Distance
  bool enteredFromLeft;
};

struct CarSpawnedEvent {
  class Car *car;
};

struct AssignPathEvent {
  class Car *car;
  std::vector<struct Waypoint> path;
};

struct CarFinishedParkingEvent {
  class Car *car;
};

struct CarDespawnEvent {
  class Car *car;
};

struct SimulationSpeedChangedEvent {
  double speedMultiplier;
};
