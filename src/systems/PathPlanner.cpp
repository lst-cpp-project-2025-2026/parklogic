#include "systems/PathPlanner.hpp"
#include "config.hpp"
#include "raymath.h"
#include <cmath>

// Helper to convert art pixels to meters
static float P2M(float artPixels) {
    return artPixels / static_cast<float>(Config::ART_PIXELS_PER_METER);
}

std::vector<Waypoint> PathPlanner::GeneratePath(const Car* car, const Module* targetFac, const Spot& targetSpot) {
    std::vector<Waypoint> path;
    
    // 1. Determine Lane and Side
    // Cars moving right (positive X) use DOWN lane.
    // Cars moving left (negative X) use UP lane.
    Lane lane = (car->getVelocity().x > 0) ? Lane::DOWN : Lane::UP;
    
    // Entry logic: Up Fac -> Right (True), Down Fac -> Left (False)
    // "Up Fac" means the facility is physically "above" the road (top side).
    // "Down Fac" means it is below the road.
    bool isUpFac = targetFac->isUp(); 
    bool useRightSide = isUpFac; 

    // 2. Waypoint 1: Road Entry
    Module* parentRoad = targetFac->getParent();
    if (parentRoad) {
        path.push_back(CalculateRoadEntry(parentRoad, lane, useRightSide));
    } else {
        // Fallback: Just go to facility center if no road
        path.push_back(CalculateFacilityEntry(targetFac, useRightSide));
    }

    // 3. Waypoint 2: Facility Center / Entrance
    path.push_back(CalculateFacilityEntry(targetFac, useRightSide));

    // 4. Waypoint 3: Alignment Point
    path.push_back(CalculateAlignmentPoint(targetFac, targetSpot));

    // 5. Waypoint 4: Final Spot
    path.push_back(CalculateSpotPoint(targetFac, targetSpot));

    return path;
}

Waypoint PathPlanner::CalculateRoadEntry(const Module* road, Lane lane, bool useRightSide) {
    // Road Entry Logic moved from Modules.cpp
    
    // Most roads (Up/Down/Double Entrance) share this center logic:
    // xCenter = 142 (art pixels)
    // However, NormalRoad simply returns center.
    // We need to handle different road types or use a generic "getEntry" helper if we kept it on Module?
    // The user wanted centralization. 
    // But road geometry IS intrinsic to the module. 
    // Let's rely on the module's generic properties (position) + specific offsets here.
    
    // Actually, distinct road types have different "entry" X coordinates. 
    // NormalRoad doesn't really have an "entry" in the same way (it's just a road segment).
    // Entrances (Up/Down/Double) have the T-junction at x=142.
    
    // Let's do a dynamic checks for now, or assume standard T-junction geometry for parent of a facility.
    // Facilities are attached to EntranceRoads.
    
    Vector2 roadPos = road->worldPosition;
    float xCenter = P2M(142); // Default T-junction center
    
    // Note: NormalRoad doesn't usually parent a facility, only EntranceRoads do.
    
    float yOffset = (lane == Lane::DOWN) ? P2M(Config::LANE_OFFSET_DOWN) : P2M(Config::LANE_OFFSET_UP);
    float xOffset = useRightSide ? P2M(18) : -P2M(18);

    // If it's a generic module without specific T-junction knowledge, we might guess center?
    // But for now, we assume the 142 center for all "connector" roads.
    
    return Waypoint(Vector2Add(roadPos, {xCenter + xOffset, yOffset}), 2.5f);
}

Waypoint PathPlanner::CalculateFacilityEntry(const Module* facility, bool useRightSide) {
    // Center Logic moved from Module::getCenterWaypoint
    
    // Base position is usually the first local waypoint or center of module.
    // But we want to centralize logic.
    // Most facilities define their "main" waypoint at:
    // width/2, height for UP facilities? No, look at Modules.cpp.
    // SmallParking: 218, height/2.
    // We should probably ask the Module for its "Entry Local Point" or "Connector Point".
    // Or we keep `getEntrancePoint()` on Module but remove path logic.
    
    // To be cleaner: The Module should know physically WHERE its entrance connector is.
    // We will assume the Module still holds a list of "Structural Waypoints" (localWaypoints).
    // We just calculate the approach vector here.
    
    Vector2 basePos = {facility->getWidth()/2, facility->getHeight()/2};
    std::vector<Waypoint> localWps = facility->getLocalWaypoints(); // We need to expose this getter
    if (!localWps.empty()) {
        basePos = localWps[0].position;
    }
    
    float offset = useRightSide ? P2M(18) : -P2M(18);
    Vector2 offsetVec = {offset, 0};
    
    return Waypoint(Vector2Add(facility->worldPosition, Vector2Add(basePos, offsetVec)), 1.5f);
}

Waypoint PathPlanner::CalculateAlignmentPoint(const Module* facility, const Spot& spot) {
    // Alignment Logic
    Vector2 spotGlobal = Vector2Add(facility->worldPosition, spot.localPosition);
    
    float backAngle = spot.orientation + PI; // Opposite direction
    float dist = 8.0f; 
    
    Vector2 offset = { cosf(backAngle) * dist, sinf(backAngle) * dist };
    Vector2 alignPos = Vector2Add(spotGlobal, offset);
    
    return Waypoint(alignPos, 1.0f);
}

Waypoint PathPlanner::CalculateSpotPoint(const Module* facility, const Spot& spot) {
    Vector2 spotGlobal = Vector2Add(facility->worldPosition, spot.localPosition);
    // Strict tolerance and StopAtEnd
    return Waypoint(spotGlobal, 0.2f, spot.id, spot.orientation, true);
}
