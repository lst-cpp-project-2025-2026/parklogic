#pragma once
#include "core/EventBus.hpp"
#include "entities/map/World.hpp"
#include "entities/map/Modules.hpp"
#include "entities/Car.hpp"
#include <memory>
#include <vector>

class EntityManager {
public:
    explicit EntityManager(std::shared_ptr<EventBus> bus);
    ~EntityManager();

    void update(double dt);
    void draw();

    // Entity Management
    void setWorld(std::unique_ptr<World> world);
    void addModule(std::unique_ptr<Module> module);
    void addCar(std::unique_ptr<Car> car);
    
    // Accessors
    World* getWorld() const { return world.get(); }
    const std::vector<std::unique_ptr<Module>>& getModules() const { return modules; }
    const std::vector<std::unique_ptr<Car>>& getCars() const { return cars; }
    
    // Clear all entities
    void clear();

    // Remove specific car
    void removeCar(Car* car);

private:
    std::shared_ptr<EventBus> eventBus;
    std::vector<Subscription> eventTokens;
    
    std::unique_ptr<World> world;
    std::vector<std::unique_ptr<Module>> modules;
    std::vector<std::unique_ptr<Car>> cars;
};
