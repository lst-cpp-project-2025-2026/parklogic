#include "systems/PathPlanner.hpp"
#include "config.hpp"
#include "raymath.h"
#include <cmath>

// --- Constants for Waypoint Geometry ---

// The horizontal center of the T-junction on an entrance road (in art pixels).
// This is where the vertical access road meets the horizontal main road.
static constexpr float ROAD_TJUNCTION_CENTER_X = 142.0f;

// The horizontal distance from the T-junction center to the Entry/Exit lanes (in art pixels).
// The total separation between the two vertical lanes is 36 pixels (18 * 2).
static constexpr float ENTRANCE_LANE_OFFSET_X = 18.0f; 

// Helper to convert art pixels to meters
static float P2M(float artPixels) {
    return artPixels / static_cast<float>(Config::ART_PIXELS_PER_METER);
}

std::vector<Waypoint> PathPlanner::GeneratePath(const Car* car, const Module* targetFac, const Spot& targetSpot) {
    std::vector<Waypoint> path;
    
    // 1. Determine Horizontal Lane on the Main Road
    // Cars moving right (positive X) use the DOWN lane (bottom lane).
    // Cars moving left (negative X) use the UP lane (top lane).
    Lane mainRoadLane = (car->getVelocity().x > 0) ? Lane::DOWN : Lane::UP;
    
    // 2. Determine Facility Orientation and Entry Side
    // UP Facilities (physically above the road) require entering from the RIGHT side of the access road.
    // DOWN Facilities (physically below the road) require entering from the LEFT side of the access road.
    bool isUpFacility = targetFac->isUp(); 
    bool useRightSideEntry = isUpFacility; // Up -> Right, Down -> Left

    // 3. Waypoint 1: Road Entry Point
    // This is the point on the main road where the car turns into the facility's access road.
    Module* parentRoad = targetFac->getParent();
    if (parentRoad) {
        path.push_back(CalculateRoadEntry(parentRoad, mainRoadLane, useRightSideEntry));
    } else {
        // Fallback: Direct approach if no parent road is defined (should not happen in proper setup)
        path.push_back(CalculateFacilityEntry(targetFac, useRightSideEntry));
    }

    // 4. Waypoint 2: Facility Entry Point
    // This is the point inside the facility's entrance, aligned with the road entry point.
    path.push_back(CalculateFacilityEntry(targetFac, useRightSideEntry));

    // 5. Waypoint 3: Alignment Point
    // A helper point in front of the parking spot to ensure a straight approach.
    path.push_back(CalculateAlignmentPoint(targetFac, targetSpot));

    // 6. Waypoint 4: Final Parking Spot
    // The exact center of the designated parking spot.
    path.push_back(CalculateSpotPoint(targetFac, targetSpot));

    return path;
}

Waypoint PathPlanner::CalculateRoadEntry(const Module* road, Lane roadLane, bool useRightSideEntry) {
    // The "Road Entry" is one of 4 possible points on the Entrance Module.
    // It is defined by the intersection of:
    // 1. The Horizontal Lane (UP or DOWN)
    // 2. The Vertical Entry Lane (Left or Right of the T-junction center)
    
    Vector2 roadWorldPos = road->worldPosition;
    
    // Horizontal Position (X):
    // Based on the T-junction center +/- the entrance lane offset.
    // Up Facility   -> Enter on Right side -> Center + 18
    // Down Facility -> Enter on Left side  -> Center - 18
    float xCenter = P2M(ROAD_TJUNCTION_CENTER_X);
    float xOffset = useRightSideEntry ? P2M(ENTRANCE_LANE_OFFSET_X) : -P2M(ENTRANCE_LANE_OFFSET_X);
    
    // Vertical Position (Y):
    // Based on the main road lane logic defined in Config.
    float yOffset = (roadLane == Lane::DOWN) ? P2M(Config::LANE_OFFSET_DOWN) : P2M(Config::LANE_OFFSET_UP);

    // Combine to get the exact turning point on the main road
    return Waypoint(Vector2Add(roadWorldPos, {xCenter + xOffset, yOffset}), 2.5f);
}

Waypoint PathPlanner::CalculateFacilityEntry(const Module* facility, bool useRightSideEntry) {
    // The "Facility Entry" aligns with the Road Entry but is physically inside the facility.
    // It serves as the anchor point for the 90-degree turn into the facility.
    
    // We start from the facility's "Base" entrance point.
    // Typically, this is the center of the module or the first local waypoint.
    Vector2 basePos = {facility->getWidth()/2, facility->getHeight()/2};
    std::vector<Waypoint> localWps = facility->getLocalWaypoints();
    if (!localWps.empty()) {
        basePos = localWps[0].position;
    }
    
    // We apply the SAME Horizontal Offset as the Road Entry to ensure vertical alignment.
    // Up Facility (Right Entry)   -> Shift Right (+18)
    // Down Facility (Left Entry)  -> Shift Left (-18)
    float offset = useRightSideEntry ? P2M(ENTRANCE_LANE_OFFSET_X) : -P2M(ENTRANCE_LANE_OFFSET_X);
    Vector2 offsetVec = {offset, 0};
    
    return Waypoint(Vector2Add(facility->worldPosition, Vector2Add(basePos, offsetVec)), 1.5f);
}

Waypoint PathPlanner::CalculateAlignmentPoint(const Module* facility, const Spot& spot) {
    // Calculates a point ~8 meters in front of the spot, aligned with the spot's orientation.
    // This forces the car to "pull up" straight before making the final approach.
    
    Vector2 spotGlobal = Vector2Add(facility->worldPosition, spot.localPosition);
    
    // "Back" direction is opposite to the spot's orientation.
    float backAngle = spot.orientation + PI; 
    float dist = 8.0f; 
    
    Vector2 offset = { cosf(backAngle) * dist, sinf(backAngle) * dist };
    Vector2 alignPos = Vector2Add(spotGlobal, offset);
    
    return Waypoint(alignPos, 1.0f);
}

Waypoint PathPlanner::CalculateSpotPoint(const Module* facility, const Spot& spot) {
    Vector2 spotGlobal = Vector2Add(facility->worldPosition, spot.localPosition);
    
    // Final destination: High precision required (0.2m tolerance), Stop at End = true.
    return Waypoint(spotGlobal, 0.2f, spot.id, spot.orientation, true);
}

std::vector<Waypoint> PathPlanner::GenerateExitPath(const Car* car, const Module* currentFac, const Spot& currentSpot, bool exitRight, float finalX) {
    std::vector<Waypoint> path;
    
    // 1. Waypoint 1: Alignment Point (Reverse)
    // Car backs out/aligns to this point first.
    path.push_back(CalculateAlignmentPoint(currentFac, currentSpot));
    
    // 2. Waypoint 2: Facility Exit Point
    // Logic:
    // UP Facility: Enters Right, Exits Left.
    // DOWN Facility: Enters Left, Exits Right.
    bool isUpFac = currentFac->isUp();
    bool useRightSideExit = !isUpFac; // Up -> Left (false), Down -> Right (true)
    
    path.push_back(CalculateFacilityEntry(currentFac, useRightSideExit));
    
    // 3. Waypoint 3: Road Entry/Exit Point
    // Logic:
    // Exiting Right -> Drive in Down Lane -> Connector is Right Side (+18)
    // Exiting Left  -> Drive in Up Lane   -> Connector is Left Side (-18)
    
    Module* parentRoad = currentFac->getParent();
    if (parentRoad) {
        Lane exitLane = exitRight ? Lane::DOWN : Lane::UP;
        
        // If exiting Right (Down Lane), we need to hit the Right connector (+18)
        // If exiting Left (Up Lane), we need to hit the Left connector (-18)
        bool roadConnectorSide = exitRight; 
        
        path.push_back(CalculateRoadEntry(parentRoad, exitLane, roadConnectorSide));
    }
    
    // 4. Waypoint 4: Map Edge Exit (Out of world)
    float yPos = 0.0f;
    if (parentRoad) {
        float laneOffset = (exitRight) ? P2M(Config::LANE_OFFSET_DOWN) : P2M(Config::LANE_OFFSET_UP);
        yPos = parentRoad->worldPosition.y + laneOffset;
    } else {
        // Fallback
        yPos = car->getPosition().y; 
    }
    
    // finalX is in Meters (passed from TrafficSystem)
    float xPos = finalX;
    
    // Stop at the map edge (or beyond)
    path.push_back(Waypoint({xPos, yPos}, 1.0f, -1, 0.0f, true));
    
    return path;
}
