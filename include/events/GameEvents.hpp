#pragma once
#include <vector>
#include "entities/map/Waypoint.hpp"
#include "raylib.h"

struct MapConfig {
  int smallParkingCount = 2;
  int largeParkingCount = 2;
  int smallChargingCount = 2;
  int largeChargingCount = 2;
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
};

struct CarSpawnedEvent {
    class Car* car;
};

struct AssignPathEvent {
    class Car* car;
    std::vector<struct Waypoint> path;
};

struct CarFinishedParkingEvent {
    class Car* car;
};

struct CarDespawnEvent {
    class Car* car;
};
