#pragma once
#include "core/EventBus.hpp"
#include "raylib.h"
#include <memory>
#include <vector>
#include <set>

class CameraSystem {
public:
    explicit CameraSystem(std::shared_ptr<EventBus> bus);
    ~CameraSystem();

    void update(double dt);
    
    // Set the world boundaries for clamping
    void setWorldBounds(float width, float height);
    
    // Get the underlying Raylib camera
    Camera2D getCamera() const { return camera; }
    
    // Setters for initial setup
    void setTarget(Vector2 target) { camera.target = target; }
    void setOffset(Vector2 offset) { camera.offset = offset; }
    void setZoom(float zoom) { camera.zoom = zoom; }

private:
    std::shared_ptr<EventBus> eventBus;
    std::vector<Subscription> eventTokens;
    
    Camera2D camera = {{0,0}, {0,0}, 0.0f, 1.0f};
    
    float worldWidth = 0.0f;
    float worldHeight = 0.0f;
    bool boundsSet = false;
    
    std::set<int> keysDown;
    double speedMultiplier = 1.0;
};
