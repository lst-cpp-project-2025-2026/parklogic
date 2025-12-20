#include "core/EntityManager.hpp"
#include "core/Logger.hpp"

#include "core/EntityManager.hpp"
#include "core/Logger.hpp"
#include "entities/map/WorldGenerator.hpp"
#include "events/GameEvents.hpp"
#include "entities/Car.hpp"

EntityManager::EntityManager(std::shared_ptr<EventBus> bus) : eventBus(bus) {
    // Subscribe to GenerateWorldEvent
    eventTokens.push_back(eventBus->subscribe<GenerateWorldEvent>([this](const GenerateWorldEvent& e) {
        Logger::Info("Generating World...");
        auto generated = WorldGenerator::generate(e.config);
        this->setWorld(std::move(generated.world));
        
        for (auto& mod : generated.modules) {
            this->addModule(std::move(mod));
        }
        
        // Publish WorldBounds
        if (world) {
            eventBus->publish(WorldBoundsEvent{world->getWidth(), world->getHeight()});
        }
    }));

    // Subscribe to GameUpdateEvent
    eventTokens.push_back(eventBus->subscribe<GameUpdateEvent>([this](const GameUpdateEvent& e) {
        this->update(e.dt);
    }));

    // Subscribe to DrawWorldEvent
    eventTokens.push_back(eventBus->subscribe<DrawWorldEvent>([this](const DrawWorldEvent&) {
        this->draw();
    }));

    // Subscribe to CreateCarEvent
    eventTokens.push_back(eventBus->subscribe<CreateCarEvent>([this](const CreateCarEvent& e) {
        if (!world) return;
        
        auto car = std::make_unique<Car>(e.position, world.get(), e.velocity, static_cast<Car::CarType>(e.carType));
        Car *carPtr = car.get();
        this->addCar(std::move(car));

        // Notify that a car has spawned
        eventBus->publish(CarSpawnedEvent{carPtr});
    }));

    // Subscribe to AssignPathEvent
    eventTokens.push_back(eventBus->subscribe<AssignPathEvent>([](const AssignPathEvent& e) {
        if (e.car) {
            e.car->setPath(e.path);
        }
    }));
}

EntityManager::~EntityManager() {
    clear();
}

void EntityManager::update(double dt) {
    if (world) {
        world->update(dt);
    }
    
    // Update Cars
    // Note: Cars need access to other cars for collision avoidance/logic
    // Currently Car::updateWithNeighbors takes a vector of unique_ptr<Car>
    // We might need to refactor Car::updateWithNeighbors to take a raw pointer list or reference to the vector
    for (auto& car : cars) {
        car->updateWithNeighbors(dt, &cars);
    }
}

void EntityManager::draw() {
    if (world) {
        world->draw();
    }
    
    for (const auto& mod : modules) {
        mod->draw();
    }
    
    for (const auto& car : cars) {
        car->draw();
    }
    
    // Draw Mask last (Foreground)
    if (world) {
        world->drawOverlay();
        world->drawMask();
    }
}

void EntityManager::setWorld(std::unique_ptr<World> w) {
    world = std::move(w);
}

void EntityManager::addModule(std::unique_ptr<Module> module) {
    modules.push_back(std::move(module));
}

void EntityManager::addCar(std::unique_ptr<Car> car) {
    cars.push_back(std::move(car));
}

void EntityManager::clear() {
    cars.clear();
    modules.clear();
    world.reset();
}

void EntityManager::removeCar(Car* car) {
    if (!car) return;
    std::erase_if(cars, [car](const std::unique_ptr<Car>& ptr) {
        return ptr.get() == car;
    });
}
