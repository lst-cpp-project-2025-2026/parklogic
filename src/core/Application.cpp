#include "core/Application.hpp"
#include "core/Logger.hpp"
#include "events/WindowEvents.hpp"

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
}

void Application::run() {
  gameLoop->run([this](double dt) { this->update(dt); }, [this]() { this->render(); }, [this]() { return isRunning; });
}

void Application::update(double dt) {
  if (window->shouldClose()) {
    eventBus->publish(WindowCloseEvent{});
  }
  inputSystem->update();
  sceneManager->update(dt);
}

void Application::render() {
  window->beginDrawing();
  sceneManager->render();

  window->endDrawing();
}
