#pragma once
#include <format>
#include <iostream>
#include <mutex>
#include <string>

/**
 * @class Logger
 * @brief A thread-safe logging utility.
 *
 * Provides static methods to log messages with different severity levels (Info, Warning, Error).
 * Supports formatted strings using std::format.
 */
class Logger {
public:
  /**
   * @brief Log severity levels.
   */
  enum class Level { Info, Warning, Error };

  /**
   * @brief Logs a raw message with a specific severity level.
   *
   * @param level The severity level.
   * @param message The message string.
   */
  static void Log(Level level, const std::string &message) {
    std::scoped_lock lock(mutex);
    switch (level) {
    case Level::Info:
      std::cout << "[INFO]  ";
      break;
    case Level::Warning:
      std::cout << "[WARN]  ";
      break;
    case Level::Error:
      std::cerr << "[ERROR] ";
      break;
    }
    std::cout << message << "\n";
  }

  /**
   * @brief Logs an informational message with formatting.
   *
   * @tparam Args Variadic template arguments for formatting.
   * @param fmt The format string.
   * @param args The arguments to format.
   */
  template <typename... Args> static void Info(std::format_string<Args...> fmt, Args &&...args) {
    Log(Level::Info, std::format(fmt, std::forward<Args>(args)...));
  }

  /**
   * @brief Logs an error message with formatting.
   *
   * @tparam Args Variadic template arguments for formatting.
   * @param fmt The format string.
   * @param args The arguments to format.
   */
  template <typename... Args> static void Error(std::format_string<Args...> fmt, Args &&...args) {
    Log(Level::Error, std::format(fmt, std::forward<Args>(args)...));
  }

private:
  static inline std::mutex mutex; ///< Mutex for thread safety.
};
