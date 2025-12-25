#include "ui/DashboardOverlay.hpp"
#include "config.hpp"
#include "raymath.h"
#include "events/InputEvents.hpp"
#include <format>
#include <string>

DashboardOverlay::DashboardOverlay(std::shared_ptr<EventBus> bus, EntityManager* em) 
    : UIElement({0, 0}, {0, 0}, bus), entityManager(em) {
    
    // Subscribe to selection events
    eventTokens.push_back(bus->subscribe<EntitySelectedEvent>([this](const EntitySelectedEvent& e) {
        currentSelection = e;
        // If something is selected, ensure it's visible? Or keep user preference?
        // Let's force visible on selection for better UX
        if (e.type != SelectionType::GENERAL) {
            visible = true;
        }
    }));

    // Subscribe to Toggle Event
    eventTokens.push_back(bus->subscribe<ToggleDashboardEvent>([this](const ToggleDashboardEvent&) {
        visible = !visible;
    }));

    // Subscribe to Toggle Key (I)
    eventTokens.push_back(bus->subscribe<KeyPressedEvent>([this](const KeyPressedEvent& e) {
        if (e.key == KEY_I) {
            eventBus->publish(ToggleDashboardEvent{});
        }
    }));

    // Default to general info
    currentSelection.type = SelectionType::GENERAL;
}

DashboardOverlay::~DashboardOverlay() {
    // Token automatically handles unsubscription
}

void DashboardOverlay::update(double /*dt*/) {
    // No specific update logic needed for now
}

void DashboardOverlay::draw() {
    if (!visible) return;

    int screenWidth = Config::LOGICAL_WIDTH;
    int panelWidth = 300;
    int pad = 20;

    int x = screenWidth - panelWidth - pad;
    int y = pad;

    // Calculate dynamic height based on content
    int estimatedHeight = 0;
    int headerHeight = 30;
    int lineUnknown = 25;
    
    // Rough estimation per type
    if (currentSelection.type == SelectionType::GENERAL) {
        estimatedHeight = headerHeight + 10 + 25 + (3 * 25) + 10 + 25 + 25 + (3 * 25); // ~350
    } else if (currentSelection.type == SelectionType::CAR) {
        estimatedHeight = headerHeight + (5 * 25); // ~155
        if (currentSelection.car && currentSelection.car->getType() == Car::CarType::ELECTRIC) estimatedHeight += 25;
    } else if (currentSelection.type == SelectionType::FACILITY) {
        estimatedHeight = headerHeight + (8 * 25); // ~230
    } else if (currentSelection.type == SelectionType::SPOT) {
        estimatedHeight = headerHeight + (3 * 25); // ~105
    }
    
    // Add padding
    estimatedHeight += 30; 

    // Draw Background
    DrawRectangle(x, y, panelWidth, estimatedHeight, Fade(BLACK, 0.8f));
    DrawRectangleLines(x, y, panelWidth, estimatedHeight, DARKGRAY);

    int contentX = x + 15;
    int contentY = y + 15;
    int contentWidth = panelWidth - 30;

    switch (currentSelection.type) {
        case SelectionType::CAR:
            drawCarInfo(contentX, contentY, contentWidth);
            break;
        case SelectionType::FACILITY:
            drawFacilityInfo(contentX, contentY, contentWidth);
            break;
        case SelectionType::SPOT:
            drawSpotInfo(contentX, contentY, contentWidth);
            break;
        case SelectionType::GENERAL:
        default:
            drawGeneralInfo(contentX, contentY, contentWidth);
            break;
    }
}

void DashboardOverlay::drawGeneralInfo(int x, int y, int width) {
    DrawText("GENERAL INFO", x, y, 20, GOLD);
    y += 30;

    int totalFacilities = 0;
    int totalSpots = 0;
    int occupiedSpots = 0;
    
    int chargingStations = 0;
    int parkingLots = 0;
    
    int chargingSpots = 0;
    int occupiedCharging = 0;
    
    int parkingSpots = 0;
    int occupiedParking = 0;

    if (entityManager) {
        auto& modules = entityManager->getModules();
        totalFacilities = modules.size();
        
        for(const auto& m : modules) {
            auto counts = m->getSpotCounts();
            totalSpots += (counts.free + counts.reserved + counts.occupied);
            occupiedSpots += counts.occupied; // Should we count reserved as occupied? PROMPT: "occupancy rate (both counting and not counting reserved spots)"
            
            // Determine type for breakdown
            auto type = m->getType();
            bool isCharging = (type == ModuleType::SMALL_CHARGING || type == ModuleType::LARGE_CHARGING);
            bool isParking = (type == ModuleType::SMALL_PARKING || type == ModuleType::LARGE_PARKING);
            
            if(isCharging) {
                chargingStations++;
                chargingSpots += (counts.free + counts.reserved + counts.occupied);
                occupiedCharging += counts.occupied;
            } else if(isParking) {
                parkingLots++;
                parkingSpots += (counts.free + counts.reserved + counts.occupied);
                occupiedParking += counts.occupied;
            }
        }
    }

    auto drawStat = [&](const char* label, const std::string& val) {
        DrawText(label, x, y, 20, WHITE);
        DrawText(val.c_str(), x + width - MeasureText(val.c_str(), 20), y, 20, GREEN);
        y += 25;
    };

    drawStat("Facilities:", std::format("{}", totalFacilities));
    drawStat("Pk Lots:", std::format("{}", parkingLots));
    drawStat("Chrg Stns:", std::format("{}", chargingStations));
    
    y += 10;
    DrawText("OCCUPANCY", x, y, 20, YELLOW);
    y += 25;
    
    float overallOcc = totalSpots > 0 ? (float)occupiedSpots / totalSpots * 100.0f : 0.0f;
    float parkingOcc = parkingSpots > 0 ? (float)occupiedParking / parkingSpots * 100.0f : 0.0f;
    float chargingOcc = chargingSpots > 0 ? (float)occupiedCharging / chargingSpots * 100.0f : 0.0f;

    drawStat("Overall:", std::format("{:.1f}%", overallOcc));
    drawStat("Parking:", std::format("{:.1f}%", parkingOcc));
    drawStat("Charging:", std::format("{:.1f}%", chargingOcc));
}

void DashboardOverlay::drawCarInfo(int x, int y, int width) {
    if (!currentSelection.car) return;
    auto* car = currentSelection.car;

    DrawText("CAR INFO", x, y, 20, GOLD);
    y += 30;
    
    auto drawStat = [&](const char* label, const std::string& val) {
        DrawText(label, x, y, 20, WHITE);
        DrawText(val.c_str(), x + width - MeasureText(val.c_str(), 20), y, 20, GREEN);
        y += 25;
    };

    std::string typeStr = (car->getType() == Car::CarType::ELECTRIC) ? "Electric" : "Gas";
    drawStat("Type:", typeStr);
    
    std::string stateStr;
    switch(car->getState()) {
        case Car::CarState::DRIVING: stateStr = "Driving"; break;
        case Car::CarState::ALIGNING: stateStr = "Parking"; break;
        case Car::CarState::PARKED: stateStr = "Parked"; break;
        case Car::CarState::EXITING: stateStr = "Exiting"; break;
    }
    drawStat("State:", stateStr);
    
    float speed = Vector2Length(car->getVelocity());
    drawStat("Speed:", std::format("{:.1f}", speed));
    
    if (car->getType() == Car::CarType::ELECTRIC) {
        drawStat("Battery:", std::format("{:.1f}%", car->getBatteryLevel()));
    }
    
    drawStat("Priority:", (car->getPriority() == Car::Priority::PRIORITY_PRICE) ? "Price" : "Distance");
}

void DashboardOverlay::drawFacilityInfo(int x, int y, int width) {
    if (!currentSelection.module) return;
    auto* m = currentSelection.module;

    DrawText("FACILITY INFO", x, y, 20, GOLD);
    y += 30;

    auto drawStat = [&](const char* label, const std::string& val) {
        DrawText(label, x, y, 20, WHITE);
        DrawText(val.c_str(), x + width - MeasureText(val.c_str(), 20), y, 20, GREEN);
        y += 25;
    };
    
    std::string typeStr = "Unknown";
    switch(m->getType()) {
        case ModuleType::SMALL_PARKING: typeStr = "Sml Parking"; break;
        case ModuleType::LARGE_PARKING: typeStr = "Lrg Parking"; break;
        case ModuleType::SMALL_CHARGING: typeStr = "Sml Charging"; break;
        case ModuleType::LARGE_CHARGING: typeStr = "Lrg Charging"; break;
        default: break;
    }
    drawStat("Type:", typeStr);
    
    auto counts = m->getSpotCounts();
    int total = counts.free + counts.reserved + counts.occupied;
    
    drawStat("Total Spots:", std::format("{}", total));
    drawStat("Free:", std::format("{}", counts.free));
    drawStat("Occupied:", std::format("{}", counts.occupied));
    drawStat("Reserved:", std::format("{}", counts.reserved));
    
    float occ = total > 0 ? (float)counts.occupied / total * 100.0f : 0.0f;
    drawStat("Occ. Rate:", std::format("{:.1f}%", occ));
    
    drawStat("Price Mult:", std::format("{:.2f}x", m->getPriceMultiplier()));
}

void DashboardOverlay::drawSpotInfo(int x, int y, int width) {
    if (!currentSelection.module || currentSelection.spotIndex == -1) return;
    auto* m = currentSelection.module;
    Spot spot = m->getSpot(currentSelection.spotIndex);

    DrawText("SPOT INFO", x, y, 20, GOLD);
    y += 30;

    auto drawStat = [&](const char* label, const std::string& val) {
        DrawText(label, x, y, 20, WHITE);
        DrawText(val.c_str(), x + width - MeasureText(val.c_str(), 20), y, 20, GREEN);
        y += 25;
    };
    
    drawStat("Index:", std::format("{}", currentSelection.spotIndex));
    
    std::string stateStr = "Free";
    if(spot.state == SpotState::RESERVED) stateStr = "Reserved";
    if(spot.state == SpotState::OCCUPIED) stateStr = "Occupied";
    drawStat("State:", stateStr);
    
    drawStat("Price:", std::format("${:.2f}", spot.price));
}
