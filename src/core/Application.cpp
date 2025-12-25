#include "core/Application.hpp"
#include "core/Logger.hpp"
#include "events/WindowEvents.hpp"
#include "events/GameEvents.hpp"

Application::Application() {
  Logger::Info("Application Starting...");

  // Initialize core systems
  eventBus = std::make_shared<EventBus>();
  window = std::make_unique<Window>(eventBus);
  inputSystem = std::make_unique<InputSystem>(eventBus, *window);
  sceneManager = std::make_unique<SceneManager>(eventBus);
  eventLogger = std::make_unique<EventLogger>(eventBus);
  gameLoop = std::make_unique<GameLoop>();

  // Start with the main menu
  sceneManager->setScene(SceneType::MainMenu);

  // Subscribe to the WindowCloseEvent to stop the application loop
  closeEventToken = eventBus->subscribe<WindowCloseEvent>([this](const WindowCloseEvent &) {
    Logger::Info("Window Close Event Received - Stopping Loop");
    isRunning = false;
  });

  // Subscribe to Simulation Speed Changes
  eventTokens.push_back(eventBus->subscribe<SimulationSpeedChangedEvent>([this](const SimulationSpeedChangedEvent &e) {
    gameLoop->setSpeedMultiplier(e.speedMultiplier);
  }));

  // Subscribe to Scene Changes to reset speed
  eventTokens.push_back(eventBus->subscribe<SceneChangeEvent>([this](const SceneChangeEvent &e) {
    if (e.newScene != SceneType::Game) {
      gameLoop->setSpeedMultiplier(1.0);
    }
  }));
}

void Application::run() {
  gameLoop->run([this](double dt) { this->update(dt); }, [this]() { this->render(); }, [this]() { return isRunning; });
}

void Application::update(double dt) {
  sceneManager->update(dt);
}

void Application::render() {
  if (window->shouldClose()) {
    eventBus->publish(WindowCloseEvent{});
  }
  inputSystem->update();

  window->beginDrawing();
  sceneManager->render();

  window->endDrawing();
}
