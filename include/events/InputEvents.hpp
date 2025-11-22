#pragma once
#include "raylib.h"
struct KeyPressedEvent {
  int key;
};
struct MouseMovedEvent {
  Vector2 position;
};
struct MouseClickEvent {
  int button;
  Vector2 position;
  bool down;
};
struct KeyReleasedEvent {
  int key;
};
