#include "core/Application.hpp"
#include "core/Logger.hpp"
#include <exception>

/**
 * @brief Main entry point of the application.
 *
 * Initializes the Application instance and runs the game loop.
 * Catches and logs any unhandled exceptions.
 *
 * @return 0 on success, -1 on error.
 */
int main() {
  try {
    Application app;
    app.run();
  } catch (const std::exception &e) {
    Logger::Error("Fatal Error: {}", e.what());
    return -1;
  } catch (...) {
    Logger::Error("Unknown Fatal Error");
    return -1;
  }
  Logger::Info("Application Exited Cleanly");
  return 0;
}
