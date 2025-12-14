#pragma once
#include "entities/map/Waypoint.hpp"
#include "entities/map/Modules.hpp"
#include "entities/Car.hpp"
#include "config.hpp"
#include <vector>

class PathPlanner {
public:
    /**
     * @brief Constructs a complete path for a car to reach a specific spot in a facility.
     * 
     * @param car The car entity (used for velocity/state).
     * @param targetFac The target facility module.
     * @param targetSpot The specific spot within the facility.
     * @return std::vector<Waypoint> The ordered list of waypoints.
     */
    static std::vector<Waypoint> GeneratePath(const Car* car, const Module* targetFac, const Spot& targetSpot);

    /**
     * @brief Constructs a path for a car to leave the facility and map.
     * @param finalX The X coordinate (in Meters) where the car should exit the map.
     */
    static std::vector<Waypoint> GenerateExitPath(const Car* car, const Module* currentFac, const Spot& currentSpot, bool exitRight, float finalX);


private:
    /**
     * @brief Calculates the entry waypoint on the road leading to the facility.
     * 
     * @param road The parent entrance road module.
     * @param roadLane The horizontal lane the car is currently in (UP or DOWN).
     * @param useRightSideEntry Whether to use the right-side entry (for UP facilities) or left-side (for DOWN facilities).
     */
    static Waypoint CalculateRoadEntry(const Module* road, Lane roadLane, bool useRightSideEntry);

    /**
     * @brief Calculates the facility's main entry/center waypoint.
     * 
     * @param facility The target facility.
     * @param useRightSideEntry Whether to align with the right-side entry or left-side entry.
     */
    static Waypoint CalculateFacilityEntry(const Module* facility, bool useRightSideEntry);

    /**
     * @brief Calculates the alignment waypoint (pull-up point) for a spot.
     */
    static Waypoint CalculateAlignmentPoint(const Module* facility, const Spot& spot);

    /**
     * @brief Calculates the final parking spot waypoint.
     */
    static Waypoint CalculateSpotPoint(const Module* facility, const Spot& spot);

    /**
     * @brief Adds a segment of waypoints from start to target, subdividing based on the Phase config.
     */
    static void AddSegment(std::vector<Waypoint>& path, Vector2 startPos, Waypoint target, const Config::CarAI::AIPhase& phase);
};
