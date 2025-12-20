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

        // Random Car Type
        // 50% Combustion, 50% Electric (Configurable later)
        int carType = (GetRandomValue(0, 1) == 0) ? 0 : 1; // 0: Combustion, 1: Electric

        eventBus->publish(CreateCarEvent{spawnPos, spawnVel, carType});
    }));

    // 2. Handle Car Spawned -> Calculate Path -> Publish AssignPathEvent
    eventTokens.push_back(eventBus->subscribe<CarSpawnedEvent>([this](const CarSpawnedEvent& e) {
        // Logger::Info("TrafficSystem: Calculating path for new car...");
        
        std::vector<Module *> facilities;
        const auto& modules = entityManager.getModules();
        
        Car::CarType type = e.car->getType();
        float battery = e.car->getBatteryLevel();
        
        bool seekCharging = false;
        
        if (type == Car::CarType::ELECTRIC) {
            if (battery < Config::BATTERY_LOW_THRESHOLD) {
                seekCharging = true;
            } else if (battery > Config::BATTERY_HIGH_THRESHOLD) {
                seekCharging = false;
            } else {
                // Weighted Random: Higher battery -> Lower chance to charge
                // Normalize battery between Low (30) and High (70): 0.0 to 1.0
                float t = (battery - Config::BATTERY_LOW_THRESHOLD) / (Config::BATTERY_HIGH_THRESHOLD - Config::BATTERY_LOW_THRESHOLD);
                // Probability to park (not charge) increases with battery
                if ((float)GetRandomValue(0, 100) / 100.0f < t) {
                    seekCharging = false;
                } else {
                    seekCharging = true;
                }
            }
        }
        
        // Filter Facilities
        for (const auto &mod : modules) {
            if (type == Car::CarType::COMBUSTION) {
                // Combustion: Parking Only
                if (dynamic_cast<SmallParking *>(mod.get()) || dynamic_cast<LargeParking *>(mod.get())) {
                    facilities.push_back(mod.get());
                }
            } else {
                // Electric
                if (seekCharging) {
                     if (dynamic_cast<SmallChargingStation *>(mod.get()) || dynamic_cast<LargeChargingStation *>(mod.get())) {
                        facilities.push_back(mod.get());
                    }
                } else {
                     if (dynamic_cast<SmallParking *>(mod.get()) || dynamic_cast<LargeParking *>(mod.get())) {
                        facilities.push_back(mod.get());
                    }
                }
            }
        }

        if (facilities.empty()) {
            Logger::Warn("TrafficSystem: No suitable facilities found for Car Type {} (SeekCharging: {}).", (int)type, seekCharging);
            // Fallback: If electric wanted to charge but can't, try parking?
            // Or just leave.
             // Try fallback to parking if charging failed
            if (find_if(facilities.begin(), facilities.end(), [](Module*){ return true; }) == facilities.end() && seekCharging) {
                 for (const auto &mod : modules) {
                     if (dynamic_cast<SmallParking *>(mod.get()) || dynamic_cast<LargeParking *>(mod.get())) {
                        facilities.push_back(mod.get());
                    }
                 }
            }
            
            if (facilities.empty()) {
                 Logger::Error("TrafficSystem: Absolutely no facilities found.");
                 return;
            }
        }

        // Pick random facility
        int idx = GetRandomValue(0, (int)facilities.size() - 1);
        Module *targetFac = facilities[idx];

        // --- New Spot-Based Pathfinding (via PathPlanner) ---
        
        // 1. Determine Spot
        int spotIndex = targetFac->getRandomSpotIndex();
        
        // Handle "Through Traffic" (No spots available)
        if (spotIndex == -1) {
             Logger::Info("TrafficSystem: Facility full (Free: 0). Car passing through.");
             
             // Calculate Map Bounds (Duplicated logic for now, or could act as if exiting)
             float minRoadX = std::numeric_limits<float>::max();
             float maxRoadX = std::numeric_limits<float>::lowest();
             const auto& mods = entityManager.getModules();
             for (const auto &mod : mods) {
                 if (auto *r = dynamic_cast<NormalRoad *>(mod.get())) {
                     float x = r->worldPosition.x;
                     float w = r->getWidth();
                     if (x < minRoadX) minRoadX = x;
                     if (x + w > maxRoadX) maxRoadX = x + w;
                }
             }
             if (minRoadX == std::numeric_limits<float>::max()) minRoadX = 0;
             if (maxRoadX == std::numeric_limits<float>::lowest()) maxRoadX = 100;

             // Determine direction based on velocity
             bool movingRight = e.car->getVelocity().x > 0;
             
             // Target beyond map edge
             float finalX = movingRight ? (maxRoadX + 2.0f) : (minRoadX - 2.0f);
             float yPos = e.car->getPosition().y; // Maintain current lane Y
             
             // Create direct exit path
             std::vector<Waypoint> exitPath;
             exitPath.push_back(Waypoint({finalX, yPos}, 1.0f, -1, 0.0f, true));
             
             e.car->setPath(exitPath);
             e.car->setState(Car::CarState::EXITING);
             
             eventBus->publish(AssignPathEvent{e.car, exitPath});
             return;
        }
        
        // Reserve the spot immediately
        targetFac->setSpotState(spotIndex, SpotState::RESERVED);
        
        // Log Reservation
        auto counts = targetFac->getSpotCounts();
        Logger::Info("TrafficSystem: Spot Reserved. Facility Status: [Free: {}, Reserved: {}, Occupied: {}]", 
                     counts.free, counts.reserved, counts.occupied);
        
        Spot spot = targetFac->getSpot(spotIndex);

        // 2. Generate Path
        std::vector<Waypoint> path = PathPlanner::GeneratePath(e.car, targetFac, spot);
        
        // Store context in Car so it knows where it is when it wants to leave
        e.car->setParkingContext(targetFac, spot, spotIndex);

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

            // Check for Arrival (Transition RESERVED -> OCCUPIED)
            // If car is ALIGNING or PARKED, it has physically arrived at the spot.
            // We need to ensure the spot reflects this.
            // We can check if state is RESERVED, then switch to OCCUPIED.
            if (car->getState() == Car::CarState::ALIGNING || car->getState() == Car::CarState::PARKED) {
                 Module* fac =  const_cast<Module*>(car->getParkedFacility());
                 int idx = car->getParkedSpotIndex();
                 if (fac && idx != -1) {
                     Spot s = fac->getSpot(idx);
                     if (s.state == SpotState::RESERVED) {
                         fac->setSpotState(idx, SpotState::OCCUPIED);
                         auto counts = fac->getSpotCounts();
                         Logger::Info("TrafficSystem: Spot Occupied. Facility Status: [Free: {}, Reserved: {}, Occupied: {}]", 
                                      counts.free, counts.reserved, counts.occupied);
                     }
                 }
            }

            // Handle Parked Logic (Charging vs Waiting)
            bool shouldExit = false;
            
            if (car->getState() == Car::CarState::PARKED) {
                Module* fac =  const_cast<Module*>(car->getParkedFacility());
                
                // Identify if charging
                bool isChargingSpot = false;
                if (dynamic_cast<SmallChargingStation*>(fac) || dynamic_cast<LargeChargingStation*>(fac)) {
                    isChargingSpot = true;
                }
                
                if (isChargingSpot && car->getType() == Car::CarType::ELECTRIC) {
                    // Charging Logic
                    car->charge(Config::CHARGING_RATE * (float)e.dt);
                    float bat = car->getBatteryLevel();
                    
                    if (bat > Config::BATTERY_FORCE_EXIT_THRESHOLD) {
                        shouldExit = true;
                    } else if (bat > Config::BATTERY_EXIT_THRESHOLD) {
                        // Chance to leave increases with percentage (80 to 95 range)
                        // normalized 0.0 to 1.0
                        float range = Config::BATTERY_FORCE_EXIT_THRESHOLD - Config::BATTERY_EXIT_THRESHOLD;
                        float excess = bat - Config::BATTERY_EXIT_THRESHOLD;
                        // float chance = (excess / range) * 0.01f; // Unused
                        
                        // Let's just do a simple check: 
                        // Base chance factor * excess * dt
                        float probability = 0.5f * (excess / range) * (float)e.dt; 
                        if ((float)GetRandomValue(0, 10000)/10000.0f < probability) {
                            shouldExit = true;
                        }
                    }
                    
                    // Keep timer high so it doesn't expire by time?
                    // Or allow time to also expire (e.g. done with coffee but car not full?)
                    // User said: "at 95% the car leaves if it hasnt" implying battery is the driver.
                    // But if parkingTimer expires, maybe they leave anyway?
                    // Let's assume Charging overrides parking timer logic, OR they execute in parallel.
                    // "if charging level is lower than 30% they go to charging"
                    // "min parking duration and a max... cars stay somewhere in between"
                    // This creates a conflict if min parking > charging time.
                    // Let's allow `shouldExit` to trigger if EITHER condition is met?
                    // Or Charging logic is exclusive for charging stations.
                    // "electric cars... charging stations... charge level increases... above 80% chance to leave"
                    // This implies the standard parking timer might NOT apply to charging stations.
                    // I will strictly use battery logic for charging stations.
                    
                } else {
                    // Regular Parking (Combustion or Electric in Parking Lot)
                    if (car->isReadyToLeave()) {
                        shouldExit = true;
                    }
                }
            }
            
            // Check if ready to leave parking
            if (shouldExit) {
                Logger::Info("TrafficSystem: Car exiting (Reason: {}).", 
                             (car->getType() == Car::CarType::ELECTRIC && car->getBatteryLevel() > 80.0f) ? "Charged" : "Timer");
                
                // 1. Context
                Module* currentFac = const_cast<Module*>(car->getParkedFacility());
                Spot currentSpot = car->getParkedSpot();
                int idx = car->getParkedSpotIndex();
                
                if (!currentFac) {
                     car->setState(Car::CarState::DRIVING); 
                     continue;
                }
                
                // Free the spot on exit!
                if (idx != -1) {
                    currentFac->setSpotState(idx, SpotState::FREE);
                    auto counts = currentFac->getSpotCounts();
                    Logger::Info("TrafficSystem: Spot Freed. Facility Status: [Free: {}, Reserved: {}, Occupied: {}]", 
                                 counts.free, counts.reserved, counts.occupied);
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
