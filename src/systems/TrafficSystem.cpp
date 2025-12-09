#include "systems/TrafficSystem.hpp"
#include "events/GameEvents.hpp"
#include "core/Logger.hpp"
#include "entities/map/Modules.hpp"
#include "config.hpp" // Added for lane offsets

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
            // Lane: Down Lane (offset 94)
            // Pos: Leftmost Road Start X, Y + Offset
            
            float laneOffset = (float)Config::LANE_OFFSET_DOWN / pixelsPerMeter;
            
            // Note: road->worldPosition is Top-Left of the module in World Coordinates.
            // Lane offset is usually relative to the top of the road asset.
            
            spawnPos.x = leftRoad->worldPosition.x; 
            spawnPos.y = leftRoad->worldPosition.y + laneOffset;
            
            spawnVel = {speed, 0};
            Logger::Info("Spawning Car LEFT at ({}, {})", spawnPos.x, spawnPos.y);

        } else {
            // Spawn Right -> Drive Left
            // Lane: Up Lane (offset 61)
            // Pos: Rightmost Road End X, Y + Offset
            
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
        Logger::Info("TrafficSystem: Calculating path for new car...");
        
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

        // Get Path
        std::vector<Waypoint> path = targetFac->getPath();

        // Publish Path Assignment
        eventBus->publish(AssignPathEvent{e.car, path});
    }));
}

TrafficSystem::~TrafficSystem() {
    eventTokens.clear();
}
