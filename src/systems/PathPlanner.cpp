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
    Lane mainRoadLane = (car->getVelocity().x > 0) ? Lane::DOWN : Lane::UP;
    
    // 2. Determine Facility Orientation and Entry Side
    bool isUpFacility = targetFac->isUp(); 
    bool useRightSideEntry = isUpFacility; // Up -> Right, Down -> Left

    // Track current position for segment generation
    Vector2 currentPos = car->getPosition();

    // 3. Waypoint 1: Road Entry Point
    // Phase: APPROACH
    Module* parentRoad = targetFac->getParent();
    Waypoint wpEntry = parentRoad ? CalculateRoadEntry(parentRoad, mainRoadLane, useRightSideEntry) 
                                  : CalculateFacilityEntry(targetFac, useRightSideEntry);
    
    // Set Angle
    wpEntry.entryAngle = isUpFacility ? -PI / 2.0f : PI / 2.0f;
    
    // Split the Approach:
    // If the distance to the entry is long (> 40m), drive in HIGHWAY mode first.
    // Then switch to APPROACH mode for the last 30m (where braking might occur).
    float distToEntry = Vector2Distance(currentPos, wpEntry.position);
    float approachDist = Config::CarAI::TURN_SLOWDOWN_DIST + 5.0f; // e.g. 35m
    
    if (distToEntry > approachDist + 10.0f) {
        // Create an intermediate "Pre-Approach" point
        // Distance from Entry = approachDist
        // Lerp factor = 1.0 - (approachDist / distToEntry)
        // Actually, we want the point at (dist - approachDist)
        
        float t = 1.0f - (approachDist / distToEntry);
        Vector2 prePos = Vector2Lerp(currentPos, wpEntry.position, t);
        
        // Target: Pre-Approach Point
        // Phase: HIGHWAY
        // Angle: 0 (Straight driving logic usually, or just point to next)
        // Actually, entryAngle matters for the *next* turn.
        // For this point, we are just driving straight on road.
        // Let's set it to 0 or derive from direction. 
        // But simpler: just inherit 0.
        
        Waypoint wpPre = wpEntry;
        wpPre.position = prePos;
        wpPre.entryAngle = 0.0f; // No sharp turn expected here
        wpPre.stopAtEnd = false;
        
        AddSegment(path, currentPos, wpPre, Config::CarAI::Phases::HIGHWAY);
        currentPos = prePos;
    }
    
    AddSegment(path, currentPos, wpEntry, Config::CarAI::Phases::APPROACH);
    currentPos = wpEntry.position; // Update head

    // 4. Waypoint 2: Facility Entry Point (Gate)
    // Phase: ACCESS
    Waypoint wpGate = CalculateFacilityEntry(targetFac, useRightSideEntry);
    wpGate.entryAngle = wpEntry.entryAngle; // Vertical
    
    AddSegment(path, currentPos, wpGate, Config::CarAI::Phases::ACCESS);
    currentPos = wpGate.position;

    // 5. Waypoint 3: Alignment Point
    // Phase: MANEUVER
    Waypoint wpAlign = CalculateAlignmentPoint(targetFac, targetSpot);
    wpAlign.entryAngle = targetSpot.orientation;

    AddSegment(path, currentPos, wpAlign, Config::CarAI::Phases::MANEUVER);
    currentPos = wpAlign.position;

    // 6. Waypoint 4: Final Parking Spot
    // Phase: PARKING
    Waypoint wpSpot = CalculateSpotPoint(targetFac, targetSpot);
    
    AddSegment(path, currentPos, wpSpot, Config::CarAI::Phases::PARKING);

    return path;
}

Waypoint PathPlanner::CalculateRoadEntry(const Module* road, Lane roadLane, bool useRightSideEntry) {
    Vector2 roadWorldPos = road->worldPosition;
    float xCenter = P2M(ROAD_TJUNCTION_CENTER_X);
    float xOffset = useRightSideEntry ? P2M(ENTRANCE_LANE_OFFSET_X) : -P2M(ENTRANCE_LANE_OFFSET_X);
    float yOffset = (roadLane == Lane::DOWN) ? P2M(Config::LANE_OFFSET_DOWN) : P2M(Config::LANE_OFFSET_UP);

    // Default tolerance/speed overridden by AddSegment
    return Waypoint(Vector2Add(roadWorldPos, {xCenter + xOffset, yOffset}));
}

Waypoint PathPlanner::CalculateFacilityEntry(const Module* facility, bool useRightSideEntry) {
    Vector2 basePos = {facility->getWidth()/2, facility->getHeight()/2};
    std::vector<Waypoint> localWps = facility->getLocalWaypoints();
    if (!localWps.empty()) {
        basePos = localWps[0].position;
    }
    float offset = useRightSideEntry ? P2M(ENTRANCE_LANE_OFFSET_X) : -P2M(ENTRANCE_LANE_OFFSET_X);
    Vector2 offsetVec = {offset, 0};
    
    return Waypoint(Vector2Add(facility->worldPosition, Vector2Add(basePos, offsetVec)));
}

Waypoint PathPlanner::CalculateAlignmentPoint(const Module* facility, const Spot& spot) {
    Vector2 spotGlobal = Vector2Add(facility->worldPosition, spot.localPosition);
    float backAngle = spot.orientation + PI; 
    float dist = 8.0f; 
    Vector2 offset = { cosf(backAngle) * dist, sinf(backAngle) * dist };
    Vector2 alignPos = Vector2Add(spotGlobal, offset);
    
    return Waypoint(alignPos);
}

Waypoint PathPlanner::CalculateSpotPoint(const Module* facility, const Spot& spot) {
    Vector2 spotGlobal = Vector2Add(facility->worldPosition, spot.localPosition);
    return Waypoint(spotGlobal, 0.2f, spot.id, spot.orientation, true);
}

std::vector<Waypoint> PathPlanner::GenerateExitPath(const Car* car, const Module* currentFac, const Spot& currentSpot, bool exitRight, float finalX) {
    std::vector<Waypoint> path;
    Vector2 currentPos = car->getPosition();
    
    // 1. Waypoint 1: Alignment Point (Reverse)
    // Phase: MANEUVER
    Waypoint wpAlign = CalculateAlignmentPoint(currentFac, currentSpot);
    // Spot->Align is slow
    AddSegment(path, currentPos, wpAlign, Config::CarAI::Phases::MANEUVER);
    currentPos = wpAlign.position;
    
    // 2. Waypoint 2: Facility Exit Point (Gate)
    // Phase: ACCESS
    bool isUpFac = currentFac->isUp();
    bool useRightSideExit = !isUpFac; 
    
    Waypoint wpGate = CalculateFacilityEntry(currentFac, useRightSideExit);
    wpGate.entryAngle = isUpFac ? PI / 2.0f : -PI / 2.0f;

    AddSegment(path, currentPos, wpGate, Config::CarAI::Phases::ACCESS);
    currentPos = wpGate.position;
    
    // 3. Waypoint 3: Road Entry/Exit Point
    // Phase: ACCESS
    Module* parentRoad = currentFac->getParent();
    if (parentRoad) {
        Lane exitLane = exitRight ? Lane::DOWN : Lane::UP;
        bool roadConnectorSide = !currentFac->isUp(); 
        
        Waypoint wpRoad = CalculateRoadEntry(parentRoad, exitLane, roadConnectorSide);
        wpRoad.entryAngle = exitRight ? 0.0f : PI;
        
        AddSegment(path, currentPos, wpRoad, Config::CarAI::Phases::ACCESS);
        currentPos = wpRoad.position;
    }
    
    // 4. Waypoint 4: Map Edge Exit
    // Phase: HIGHWAY (Crucial change: High density correction over long distance)
    float yPos = 0.0f;
    if (parentRoad) {
        float laneOffset = (exitRight) ? P2M(Config::LANE_OFFSET_DOWN) : P2M(Config::LANE_OFFSET_UP);
        yPos = parentRoad->worldPosition.y + laneOffset;
    } else {
        yPos = car->getPosition().y; 
    }
    
    Waypoint wpEdge({finalX, yPos}, 1.0f, -1, 0.0f, true);
    
    AddSegment(path, currentPos, wpEdge, Config::CarAI::Phases::HIGHWAY);
    
    return path;
}

void PathPlanner::AddSegment(std::vector<Waypoint>& path, Vector2 startPos, Waypoint target, const Config::CarAI::AIPhase& phase) {
    // 1. Calculate Segment distance
    float dist = Vector2Distance(startPos, target.position);
    
    // 2. Determine number of correction points
    // If step is 15m and dist is 45m -> 3 steps -> 2 intermediate points?
    // count = floor(dist / step)
    
    if (phase.correctionStep > 0.0f && dist > phase.correctionStep) {
        int steps = std::max(1, (int)(dist / phase.correctionStep));
        
        for (int k = 1; k < steps; ++k) {
            float t = (float)k / (float)steps;
            Vector2 newPos = Vector2Lerp(startPos, target.position, t);
            
            Waypoint wpCorr = target; // Inherit target properties (angle/ID potentially)
            wpCorr.position = newPos;
            wpCorr.id = -100; // Debug ID
            wpCorr.stopAtEnd = false;
            
            // Apply Phase Config
            wpCorr.tolerance = phase.tolerance;
            wpCorr.speedLimitFactor = phase.speedFactor;
            
            path.push_back(wpCorr);
        }
    }
    
    // 3. Add the actual target waypoint
    target.tolerance = phase.tolerance;
    target.speedLimitFactor = phase.speedFactor;
    
    path.push_back(target);
}
