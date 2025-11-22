#include "core/EventLogger.hpp"
#include "core/Logger.hpp"
#include "events/GameEvents.hpp"
#include "events/InputEvents.hpp"
#include "events/WindowEvents.hpp"

EventLogger::EventLogger(std::shared_ptr<EventBus> bus) : eventBus(bus) {
  // Subscribe to various events and log them
  subscriptions.push_back(eventBus->subscribe<SceneChangeEvent>(
      [](const SceneChangeEvent &e) { Logger::Info("Event: SceneChangeEvent [NewScene: {}]", (int)e.newScene); }));

  subscriptions.push_back(eventBus->subscribe<KeyPressedEvent>(
      [](const KeyPressedEvent &e) { Logger::Info("Event: KeyPressedEvent [Key: {}]", e.key); }));

  subscriptions.push_back(eventBus->subscribe<KeyReleasedEvent>(
      [](const KeyReleasedEvent &e) { Logger::Info("Event: KeyReleasedEvent [Key: {}]", e.key); }));

  subscriptions.push_back(eventBus->subscribe<MouseMovedEvent>([](const MouseMovedEvent & /*e*/) {
    // Commented out to avoid spamming logs, uncomment if needed
    // Logger::Info("Event: MouseMovedEvent [x: {}, y: {}]", e.position.x, e.position.y);
  }));

  subscriptions.push_back(
      eventBus->subscribe<GamePausedEvent>([](const GamePausedEvent &) { Logger::Info("Event: GamePausedEvent"); }));

  subscriptions.push_back(
      eventBus->subscribe<GameResumedEvent>([](const GameResumedEvent &) { Logger::Info("Event: GameResumedEvent"); }));

  subscriptions.push_back(eventBus->subscribe<BallBounceEvent>([](const BallBounceEvent &e) {
    Logger::Info("Event: BallBounceEvent [x: {}, y: {}]", e.position.x, e.position.y);
  }));

  subscriptions.push_back(eventBus->subscribe<MouseClickEvent>([](const MouseClickEvent &e) {
    Logger::Info("Event: MouseClickEvent [Button: {}, x: {}, y: {}, Down: {}]", e.button, e.position.x, e.position.y,
                 e.down);
  }));

  subscriptions.push_back(eventBus->subscribe<WindowResizeEvent>([](const WindowResizeEvent &e) {
    Logger::Info("Event: WindowResizeEvent [Width: {}, Height: {}]", e.width, e.height);
  }));

  subscriptions.push_back(
      eventBus->subscribe<WindowCloseEvent>([](const WindowCloseEvent &) { Logger::Info("Event: WindowCloseEvent"); }));
}
