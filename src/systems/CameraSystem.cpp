#include "systems/CameraSystem.hpp"
#include "events/GameEvents.hpp"
#include "events/InputEvents.hpp"
#include "config.hpp"
#include "core/Logger.hpp"

CameraSystem::CameraSystem(std::shared_ptr<EventBus> bus) : eventBus(bus) {
    // Initialize Camera Offset to center of screen
    camera.offset = {Config::LOGICAL_WIDTH / 2.0f, Config::LOGICAL_HEIGHT / 2.0f};
    camera.zoom = 1.0f;

    // Subscribe to Zoom Event
    eventTokens.push_back(eventBus->subscribe<CameraZoomEvent>([this](const CameraZoomEvent& e) {
        // Multiplicative Zoom for "more linear and granular" feel at different levels
        // e.zoomDelta is usually +/- 0.1
        // We want step to be small, e.g. 5-10%
        float factor = 1.0f + (e.zoomDelta * 1.5f); // 0.1 becomes 1.15x, -0.1 becomes 0.85x
        if (factor < 0.1f) factor = 0.1f; // Safety
        
        camera.zoom *= factor;

        if (camera.zoom < 0.1f) camera.zoom = 0.1f;
        if (camera.zoom > 5.0f) camera.zoom = 5.0f;
    }));

    // Subscribe to Move Event
    eventTokens.push_back(eventBus->subscribe<CameraMoveEvent>([this](const CameraMoveEvent& e) {
        // Correct for speed multiplier if this comes from update loop?
        // Actually CameraMoveEvent comes mainly from external (if any).
        // Standard movement is in update() below.
        camera.target.x += e.delta.x;
        camera.target.y += e.delta.y;
    }));

    // Track Keys
    eventTokens.push_back(eventBus->subscribe<KeyPressedEvent>([this](const KeyPressedEvent& e) {
        keysDown.insert(e.key);
    }));
    eventTokens.push_back(eventBus->subscribe<KeyReleasedEvent>([this](const KeyReleasedEvent& e) {
        keysDown.erase(e.key);
    }));

     // Subscribe to Speed Change
    eventTokens.push_back(eventBus->subscribe<SimulationSpeedChangedEvent>([this](const SimulationSpeedChangedEvent& e) {
        speedMultiplier = e.speedMultiplier;
        if (speedMultiplier < 0.1) speedMultiplier = 1.0; // Safety
    }));

    // Subscribe to GameUpdateEvent
    eventTokens.push_back(eventBus->subscribe<GameUpdateEvent>([this](const GameUpdateEvent& e) {
        this->update(e.dt);
    }));

    // Subscribe to WorldBoundsEvent
    eventTokens.push_back(eventBus->subscribe<WorldBoundsEvent>([this](const WorldBoundsEvent& e) {
        this->setWorldBounds(e.width, e.height);
        // Center camera on world
        this->setTarget({e.width / 2.0f, e.height / 2.0f});
    }));

    // Subscribe to Render Events
    eventTokens.push_back(eventBus->subscribe<BeginCameraEvent>([this](const BeginCameraEvent&) {
        Camera2D renderCamera = camera;
        renderCamera.zoom *= Config::PPM;
        BeginMode2D(renderCamera);
    }));

    eventTokens.push_back(eventBus->subscribe<EndCameraEvent>([this](const EndCameraEvent&) {
        EndMode2D();
    }));
}

CameraSystem::~CameraSystem() {
    eventTokens.clear();
}

void CameraSystem::setWorldBounds(float width, float height) {
    worldWidth = width;
    worldHeight = height;
    boundsSet = true;
}

void CameraSystem::update(double dt) {
    // Handle Input
    // Compensate for Simulation Speed so camera moves at Real-Time speed
    float effectiveDt = (float)dt / (float)speedMultiplier;
    
    float speed = 20.0f / camera.zoom;
    Vector2 delta = {0, 0};

    if (keysDown.contains(KEY_W)) delta.y -= speed * effectiveDt;
    if (keysDown.contains(KEY_S)) delta.y += speed * effectiveDt;
    if (keysDown.contains(KEY_A)) delta.x -= speed * effectiveDt;
    if (keysDown.contains(KEY_D)) delta.x += speed * effectiveDt;

    camera.target.x += delta.x;
    camera.target.y += delta.y;

    // Clamp Camera Target to World Bounds
    if (boundsSet) {
        if (camera.target.x < 0) camera.target.x = 0;
        if (camera.target.y < 0) camera.target.y = 0;
        if (camera.target.x > worldWidth) camera.target.x = worldWidth;
        if (camera.target.y > worldHeight) camera.target.y = worldHeight;
    }
}
