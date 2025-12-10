#pragma once
#include "entities/map/Waypoint.hpp"
#include "entities/map/Modules.hpp"
#include "entities/Car.hpp"
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

private:
    /**
     * @brief Calculates the entry waypoint on the road leading to the facility.
     */
    static Waypoint CalculateRoadEntry(const Module* road, Lane lane, bool useRightSide);

    /**
     * @brief Calculates the facility's main entry/center waypoint.
     */
    static Waypoint CalculateFacilityEntry(const Module* facility, bool useRightSide);

    /**
     * @brief Calculates the alignment waypoint (pull-up point) for a spot.
     */
    static Waypoint CalculateAlignmentPoint(const Module* facility, const Spot& spot);

    /**
     * @brief Calculates the final parking spot waypoint.
     */
    static Waypoint CalculateSpotPoint(const Module* facility, const Spot& spot);
};
