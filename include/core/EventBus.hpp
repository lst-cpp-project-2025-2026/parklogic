#pragma once

#include "events/EventTypes.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

// Forward declaration
class EventBus;

/**
 * @brief A RAII (Resource Acquisition Is Initialization) token representing an active subscription.
 *
 * The Subscription object manages the lifecycle of a listener. When this object goes
 * out of scope, it automatically attempts to unsubscribe from the EventBus.
 *
 * This class is Move-Only. It cannot be copied, as that would imply two tokens
 * managing the same unique subscription ID.
 */
class Subscription {
public:
  Subscription() = default;

  /**
   * @brief Constructs a valid subscription token.
   * @param bus A weak reference to the EventBus to prevent circular dependency / retention cycles.
   * @param type The type index of the event being listened to.
   * @param id The unique ID assigned to the specific callback within the bus.
   */
  Subscription(std::weak_ptr<EventBus> bus, std::type_index type, size_t id)
      : weakBus(std::move(bus)), eventType(type), handlerId(id) {}

  /**
   * @brief Move Constructor.
   * Transfers ownership of the subscription from 'other' to this object.
   * 'other' is invalidated to prevent it from unsubscribing upon destruction.
   */
  Subscription(Subscription &&other) noexcept { moveFrom(std::move(other)); }

  /**
   * @brief Move Assignment Operator.
   * If this object currently holds a valid subscription, it unsubscribes first.
   * Then it takes ownership of 'other'.
   */
  Subscription &operator=(Subscription &&other) noexcept {
    if (this != &other) {
      unsubscribe();
      moveFrom(std::move(other));
    }
    return *this;
  }

  // Copying is deleted to ensure one-to-one mapping of Token <-> Callback ID.
  Subscription(const Subscription &) = delete;
  Subscription &operator=(const Subscription &) = delete;

  /**
   * @brief Destructor.
   * Automatically unsubscribes the listener if the EventBus is still alive.
   */
  ~Subscription() { unsubscribe(); }

  /**
   * @brief Manually unsubscribes the listener.
   * Can be called explicitly before scope end. Idempotent (safe to call multiple times).
   */
  void unsubscribe();

private:
  /**
   * @brief Helper to implement move semantics without code duplication.
   */
  void moveFrom(Subscription &&other) {
    weakBus = std::move(other.weakBus);
    eventType = other.eventType;
    handlerId = other.handlerId;

    // Invalidate the source to prevent double-free logic
    other.handlerId = 0;
    other.weakBus.reset();
  }

  std::weak_ptr<EventBus> weakBus;
  std::type_index eventType = std::type_index(typeid(void));
  size_t handlerId = 0;
};

/**
 * @brief A Thread-Safe, Type-Safe Event Bus system.
 *
 * Implements the Publish-Subscribe pattern using C++ RTTI (Run-Time Type Information)
 * for type erasure. It allows decoupling of Publishers and Subscribers.
 *
 * Thread Safety Model:
 * - Multiple threads can call publish() concurrently (Shared Lock).
 * - subscribe() and unsubscribe() are exclusive operations (Unique Lock).
 * - Reentrancy is supported: Callbacks can safely subscribe/unsubscribe during execution
 *   without invalidating iterators or causing deadlocks.
 */
class EventBus : public std::enable_shared_from_this<EventBus> {
public:
  using HandlerId = size_t;

  /**
   * @brief Subscribes a callback function to a specific Event type.
   *
   * @tparam T The Event type (struct or class) to listen for.
   * @param callback A std::function or lambda taking 'const T&'.
   * @return Subscription A RAII token. The subscription remains active as long as this token exists.
   */
  template <EventType T> [[nodiscard]] Subscription subscribe(std::function<void(const T &)> callback) {
    // Exclusive lock: We are modifying the internal map structure.
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto typeIdx = std::type_index(typeid(T));
    HandlerId id = nextId++;

    // Create a shared_ptr wrapper. This ensures the callback stays alive
    // even if the map is modified during a publish cycle.
    auto wrapper = std::make_shared<EventWrapper<T>>(std::move(callback));
    wrapper->id = id;

    subscribers[typeIdx].push_back(wrapper);

    return Subscription(weak_from_this(), typeIdx, id);
  }

  /**
   * @brief Publishes an event to all listeners of type T.
   *
   * This method employs a "Snapshot" strategy for thread safety and reentrancy:
   * 1. Acquires a Read Lock.
   * 2. Copies the list of subscribers (shared_ptrs) for the specific event.
   * 3. Releases the Lock.
   * 4. Iterates over the local copy to execute callbacks.
   *
   * This ensures that if a callback takes a long time or modifies the subscription list
   * (e.g., unsubscribes itself), it does not block other threads or invalidate the loop.
   *
   * @tparam T The type of the event object.
   * @param event The event data instance.
   */
  template <EventType T> void publish(const T &event) {
    std::vector<std::shared_ptr<IEventWrapper>> listenersSnapshot;

    {
      // Shared Lock: Allows concurrent calls to publish() from multiple threads.
      std::shared_lock<std::shared_mutex> lock(mutex_);

      auto typeIdx = std::type_index(typeid(T));
      auto it = subscribers.find(typeIdx);

      if (it == subscribers.end() || it->second.empty()) {
        return;
      }

      // Create a local copy (increments ref counts).
      // This keeps the handlers alive for the duration of this function call
      // even if they are removed from the main 'subscribers' map.
      listenersSnapshot = it->second;
    } // Mutex releases here

    // Execute callbacks outside the lock to prevent deadlocks if a callback
    // tries to acquire the lock again (e.g., via subscribe).
    for (const auto &wrapper : listenersSnapshot) {
      // Re-cast type-erased pointer back to the specific event wrapper
      static_cast<EventWrapper<T> *>(wrapper.get())->call(event);
    }
  }

  /**
   * @brief Internal method called by Subscription destructor.
   * Removes a specific handler ID from the subscriber list.
   */
  void unsubscribe(std::type_index type, HandlerId id) {
    // Exclusive Lock: Modifying the vector/map.
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = subscribers.find(type);
    if (it != subscribers.end()) {
      auto &list = it->second;

      // Remove the wrapper with the matching ID
      std::erase_if(list, [id](const auto &wrapper) { return wrapper->id == id; });

      // Cleanup: If no listeners remain for this type, remove the map entry
      if (list.empty()) {
        subscribers.erase(it);
      }
    }
  }

private:
  /**
   * @brief Type Erasure Base Interface.
   * Allows storing templated EventWrappers in a generic container.
   */
  struct IEventWrapper {
    HandlerId id;
    virtual ~IEventWrapper() = default;
  };

  /**
   * @brief Templated implementation of the wrapper.
   * Holds the strongly-typed callback function.
   */
  template <typename T> struct EventWrapper : IEventWrapper {
    std::function<void(const T &)> callback;
    explicit EventWrapper(std::function<void(const T &)> cb) : callback(std::move(cb)) {}
    void call(const T &event) { callback(event); }
  };

  // Storage: Map<TypeIndex, List of Wrappers>
  // We use shared_ptr to support the snapshotting mechanism in publish().
  std::unordered_map<std::type_index, std::vector<std::shared_ptr<IEventWrapper>>> subscribers;

  HandlerId nextId = 1;

  // shared_mutex allows multiple readers (publish) but only one writer (subscribe/unsubscribe)
  mutable std::shared_mutex mutex_;
};

// -----------------------------------------------------------------------------
// Inline Implementation
// -----------------------------------------------------------------------------

inline void Subscription::unsubscribe() {
  // 0 indicates a moved-from or default-constructed state (invalid).
  if (handlerId == 0)
    return;

  // Lock the weak pointer to ensure the Bus still exists.
  if (auto bus = weakBus.lock()) {
    bus->unsubscribe(eventType, handlerId);
  }

  // Reset state to prevent re-execution
  handlerId = 0;
  weakBus.reset();
}
