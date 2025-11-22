# ParkLogic

### Build & Run
```bash
mkdir build && cd build
cmake ..
cmake --build .
./ParkLogic
```

## Project Structure

- **include/**: Header files, organized by module.
    - `core/`: Core engine systems (Application, Window, EventBus, Logger).
    - `entities/`: Game objects (Entity base, Car, World).
    - `events/`: Event definitions and types.
    - `scenes/`: Scene management and specific game scenes.
    - `ui/`: UI system and elements.
- **src/**: Implementation files corresponding to the headers.

## Architecture

### Core Loop
The `GameLoop` drives the application. It manages the `SceneManager`, which in turn updates and draws the active `IScene`.
- **Update Cycle**: `GameLoop` -> `SceneManager` -> `CurrentScene::update(dt)`
- **Render Cycle**: `GameLoop` -> `SceneManager` -> `CurrentScene::draw()`

### Event System
The engine uses a type-safe, thread-safe `EventBus` for communication between decoupled systems.
- **Publishing**: `eventBus->publish(MyEvent{data});`
- **Subscribing**: Returns a `Subscription` token that keeps the listener active.
  ```cpp
  auto token = eventBus->subscribe<MyEvent>([](const MyEvent& e) {
      // Handle event
  });
  ```
- **Note**: Store your `Subscription` tokens! If they go out of scope, the listener is automatically unsubscribed.

### Entity System
All game objects inherit from the `Entity` abstract base class.
- **Interface**:
    - `update(double dt)`: Handle logic, physics, state changes.
    - `draw()`: Render the entity using Raylib functions.
- **Management**: Entities are typically managed by the active Scene (e.g., `GameScene` holds a `std::vector<std::unique_ptr<Entity>>`).

## How to Implement New Features

### 1. Adding a New Entity
1.  Create a new class in `include/entities/` inheriting from `Entity`.
2.  Implement `update(double dt)` and `draw()`.
3.  Add any specific logic (e.g., movement, collision).
4.  Instantiate it in `GameScene::load()` and add it to a container (e.g., `cars` vector or a new generic entity list).

**Example:**
```cpp
class Obstacle : public Entity {
public:
    void update(double dt) override { /* Logic */ }
    void draw() override { DrawRectangle(...); }
};
```

### 2. Modifying Entity Logic
- **Car Logic**: Check `src/entities/Car.cpp`. The `update` method handles movement, and `updateWithNeighbors` handles flocking/avoidance behavior.
- **World Logic**: Check `src/entities/World.cpp` for grid rendering and boundary management.

### 3. Adding New Events
1.  Define a new struct in a header within `include/events/` (e.g., `GameEvents.hpp`).
    ```cpp
    struct PlayerScoreEvent { int score; };
    ```
2.  Publish it from anywhere with access to the `EventBus`.
3.  Subscribe to it in the relevant system (e.g., `GameScene`, `UIManager`).

### 4. Creating a New Scene
1.  Inherit from `IScene`.
2.  Implement `load()`, `unload()`, `update()`, `draw()`.
3.  Register/Switch to it via `SceneManager`.
