#include "systems/TrafficSystem.hpp"
#include "systems/PathPlanner.hpp"
#include "events/GameEvents.hpp"
#include "core/Logger.hpp"
#include "entities/map/Modules.hpp"
#include "config.hpp" // Added for lane offsets

#include "entities/Car.hpp" // Added include

TrafficSystem::TrafficSystem(std::shared_ptr<EventBus> bus, const EntityManager& em) 
    : eventBus(bus), entityManager(em) {
    
    // 1. Handle Spawn Request -> Find Position -> Publish CreateCarEvent
    eventTokens.push_back(eventBus->subscribe<SpawnCarRequestEvent>([this](const SpawnCarRequestEvent&) {
        Logger::Info("TrafficSystem: Processing Spawn Request...");
        
        const auto& modules = entityManager.getModules();
        if (modules.empty()) return;

        // Find Leftmost and Rightmost Roads
        const Module* leftRoad = nullptr;
        const Module* rightRoad = nullptr;
        float minX = std::numeric_limits<float>::max();
        float maxRightX = std::numeric_limits<float>::lowest();

        for (const auto &mod : modules) {
            // We assume external roads are NormalRoads
             if (auto *r = dynamic_cast<NormalRoad *>(mod.get())) {
                 float x = r->worldPosition.x;
                 float w = r->getWidth();
                 
                 if (x < minX) {
                     minX = x;
                     leftRoad = r;
                 }
                 
                 if (x + w > maxRightX) {
                     maxRightX = x + w;
                     rightRoad = r;
                 }
            }
        }
        
        if (!leftRoad && !rightRoad) {
            Logger::Error("TrafficSystem: No roads found to spawn cars.");
            return;
        }

        // Randomly choose side
        bool spawnLeft = (GetRandomValue(0, 1) == 0);
        
        // If one side is missing, force the other
        if (!leftRoad) spawnLeft = false;
        if (!rightRoad) spawnLeft = true;
        
        Vector2 spawnPos = {0, 0};
        Vector2 spawnVel = {0, 0};
        float speed = 15.0f; // Initial speed (matches max speed roughly)
        
        float pixelsPerMeter = static_cast<float>(Config::ART_PIXELS_PER_METER);

        if (spawnLeft) {
            // Spawn Left -> Drive Right
            float laneOffset = (float)Config::LANE_OFFSET_DOWN / pixelsPerMeter;
            
            // Note: road->worldPosition is Top-Left of the module in World Coordinates.
            // Lane offset is usually relative to the top of the road asset.
            
            spawnPos.x = leftRoad->worldPosition.x; 
            spawnPos.y = leftRoad->worldPosition.y + laneOffset;
            
            spawnVel = {speed, 0};
            Logger::Info("Spawning Car LEFT at ({}, {})", spawnPos.x, spawnPos.y);

        } else {
            // Spawn Right -> Drive Left
            float laneOffset = (float)Config::LANE_OFFSET_UP / pixelsPerMeter;
            
            spawnPos.x = rightRoad->worldPosition.x + rightRoad->getWidth();
            spawnPos.y = rightRoad->worldPosition.y + laneOffset;
            
            spawnVel = {-speed, 0};
             Logger::Info("Spawning Car RIGHT at ({}, {})", spawnPos.x, spawnPos.y);
        }

        eventBus->publish(CreateCarEvent{spawnPos, spawnVel});
    }));

    // 2. Handle Car Spawned -> Calculate Path -> Publish AssignPathEvent
    eventTokens.push_back(eventBus->subscribe<CarSpawnedEvent>([this](const CarSpawnedEvent& e) {
        // Logger::Info("TrafficSystem: Calculating path for new car...");
        
        std::vector<Module *> facilities;
        const auto& modules = entityManager.getModules();
        for (const auto &mod : modules) {
            if (dynamic_cast<SmallParking *>(mod.get()) || dynamic_cast<LargeParking *>(mod.get()) ||
                dynamic_cast<SmallChargingStation *>(mod.get()) || dynamic_cast<LargeChargingStation *>(mod.get())) {
                facilities.push_back(mod.get());
            }
        }

        if (facilities.empty()) {
            Logger::Error("TrafficSystem: No facilities found.");
            return;
        }

        // Pick random facility
        int idx = GetRandomValue(0, (int)facilities.size() - 1);
        Module *targetFac = facilities[idx];

        // --- New Spot-Based Pathfinding (via PathPlanner) ---
        
        // 1. Determine Spot
        int spotIndex = targetFac->getRandomSpotIndex();
        if (spotIndex == -1) {
             Logger::Error("TrafficSystem: Facility has no spots.");
             return;
        }
        Spot spot = targetFac->getSpot(spotIndex);

        // 2. Generate Path
        std::vector<Waypoint> path = PathPlanner::GeneratePath(e.car, targetFac, spot);
        
        // Store context in Car so it knows where it is when it wants to leave
        e.car->setParkingContext(targetFac, spot);

        // Publish Path Assignment
        eventBus->publish(AssignPathEvent{e.car, path});
    }));
    
    // 3. Handle Game Update -> Check for Cars Ready to Leave AND Remove Exited Cars
    eventTokens.push_back(eventBus->subscribe<GameUpdateEvent>([this](const GameUpdateEvent& e) {
        // We use getCars() directly
        const auto& cars = entityManager.getCars();
        
        // List of cars to remove (pointers)
        std::vector<Car*> carsToRemove;
        
        // Calculate World Road Boundaries (in Meters)
        // We do this every frame, but ideally it should be cached or calculated once.
        // For now, it's fast enough.
        float minRoadX = std::numeric_limits<float>::max();
        float maxRoadX = std::numeric_limits<float>::lowest();
        
        const auto& modules = entityManager.getModules();
        for (const auto &mod : modules) {
             if (auto *r = dynamic_cast<NormalRoad *>(mod.get())) {
                 // Road X and Width are in Meters!
                 float x = r->worldPosition.x;
                 float w = r->getWidth();
                 if (x < minRoadX) minRoadX = x;
                 if (x + w > maxRoadX) maxRoadX = x + w;
            }
        }
        
        // If no roads, defaults
        if (minRoadX == std::numeric_limits<float>::max()) minRoadX = 0;
        if (maxRoadX == std::numeric_limits<float>::lowest()) maxRoadX = 100;
        
        for (const auto& carPtr : cars) {
            Car* car = carPtr.get();
            if (!car) continue;

            // Check if ready to leave parking
            if (car->isReadyToLeave()) {
                Logger::Info("TrafficSystem: Car finished parking. Generating exit path...");
                
                // 1. Context
                const Module* currentFac = car->getParkedFacility();
                Spot currentSpot = car->getParkedSpot();
                
                if (!currentFac) {
                     car->setState(Car::CarState::DRIVING); 
                     continue;
                }
                
                // 2. Decide Direction
                bool exitRight = (GetRandomValue(0, 1) == 1);
                
                // 3. Determine Final Destination X
                // Drive OFF screen.
                // If exiting Right, go to maxRoadX + 2.0m
                // If exiting Left, go to minRoadX - 2.0m (start of road is minX)
                float finalX = exitRight ? (maxRoadX + 2.0f) : (minRoadX - 2.0f);
                
                // 4. Generate Path
                std::vector<Waypoint> path = PathPlanner::GenerateExitPath(car, currentFac, currentSpot, exitRight, finalX);
                
                // 5. Assign
                car->setPath(path);
                car->setState(Car::CarState::EXITING);
                Logger::Info("TrafficSystem: Exit path assigned. Exiting {}", exitRight ? "RIGHT" : "LEFT");
            }
            
            // Check if finished exiting
            if (car->getState() == Car::CarState::EXITING && car->hasArrived()) {
                // Car has reached the end of the exit path (off-screen)
                carsToRemove.push_back(car);
                Logger::Info("TrafficSystem: Car has left the world. Despawning...");
            }
        }
        
        // Remove cars
        for (Car* c : carsToRemove) {
            // Need a const_cast or entityManager needs to accept Car* (it does)
            // But entityManager is const in this lambda context?
            // Member variable: const EntityManager& entityManager
            // We need a non-const EntityManager to remove.
            // The constructor takes `const EntityManager& em`.
            // Use const_cast or fix the design?
            // Fix design: EntityManager shouldn't be const if we modify it.
            // TrafficSystem owns eventBus but only has reference to EM.
            // Actually, TrafficSystem is a system. It should modify components.
            // We should cast away constness for now as we know it's the main EM.
            const_cast<EntityManager&>(entityManager).removeCar(c);
        }
    }));
}

TrafficSystem::~TrafficSystem() {
    eventTokens.clear();
}
