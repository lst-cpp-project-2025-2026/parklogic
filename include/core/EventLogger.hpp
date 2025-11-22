#pragma once
#include "core/EventBus.hpp"
#include <memory>
#include <vector>

/**
 * @class EventLogger
 * @brief Subscribes to various events and logs them for debugging purposes.
 *
 * The EventLogger is a utility class that listens to important game events
 * and prints their details to the console using the Logger.
 */
class EventLogger {
public:
  /**
   * @brief Constructs the EventLogger and subscribes to events.
   *
   * @param eventBus The EventBus to subscribe to.
   */
  explicit EventLogger(std::shared_ptr<EventBus> eventBus);

  /**
   * @brief Destructor.
   *
   * Automatically unsubscribes from all events via the stored tokens.
   */
  ~EventLogger() = default;

private:
  std::shared_ptr<EventBus> eventBus;      ///< The EventBus instance.
  std::vector<Subscription> subscriptions; ///< List of active subscription tokens.
};
