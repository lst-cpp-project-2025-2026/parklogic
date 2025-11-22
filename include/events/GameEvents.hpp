#pragma once
#include "raylib.h"
enum class SceneType { MainMenu, Game };
struct SceneChangeEvent {
  SceneType newScene;
};
struct GamePausedEvent {};
struct GameResumedEvent {};
struct BallBounceEvent {
  Vector2 position;
};
