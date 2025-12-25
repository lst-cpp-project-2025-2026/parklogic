#pragma once
#include "entities/map/Waypoint.hpp"
#include "raylib.h"
#include <vector>

struct AttachmentPoint {
  Vector2 position; // Relative to module top-left
  Vector2 normal;   // Direction of attachment
};

enum class Lane { UP, DOWN };

enum class ModuleType {
    GENERIC,
    ROAD,
    SMALL_PARKING,
    LARGE_PARKING,
    SMALL_CHARGING,
    LARGE_CHARGING
};

enum class SpotState {
    FREE,
    RESERVED,
    OCCUPIED
};

struct Spot {
    Vector2 localPosition;
    float orientation; // 0=Right, PI/2=Down, PI=Left, 3PI/2=Up (in radians)
    int id;
    SpotState state = SpotState::FREE;
    float price = 0.0f; // NEW: Price per usage
};

class Module {
public:
  Module(float w, float h) : width(w), height(h) {
      // Base random multiplier for this facility (1.0 to 3.0)
      // This makes some facilities "posh" and others "cheap"
      priceMultiplier = (float)GetRandomValue(10, 30) / 10.0f; 
  }
  virtual ~Module() = default;

  float getWidth() const { return width; }
  float getHeight() const { return height; }
  
  float getPriceMultiplier() const { return priceMultiplier; }
  void setPriceMultiplier(float m) { priceMultiplier = m; }
  
  // Helper to init spots with price
  void assignRandomPricesToSpots(float baseSpotPrice, float variance) {
      for(auto& spot : spots) {
          // Spot Price = Base * FacilityMultiplier + RandomVariance
          float r = (float)GetRandomValue(-(int)(variance*10), (int)(variance*10)) / 10.0f;
          spot.price = (baseSpotPrice * priceMultiplier) + r;
          if (spot.price < 0.5f) spot.price = 0.5f; // Min price
      }
  }

  const std::vector<AttachmentPoint> &getAttachmentPoints() const { return attachmentPoints; }


  // Position in the world (set during generation)
  Vector2 worldPosition = {0, 0};

  virtual void draw() const;

  void addWaypoint(Vector2 localPos, float tolerance = 1.0f, int id = -1, float angle = 0.0f, bool stop = false);

  std::vector<Waypoint> getGlobalWaypoints() const;

  // Hierarchy
  void setParent(Module* p) { parent = p; }
  Module* getParent() const { return parent; }

  // Recursive path retrieval (Legacy/Fallback)
  std::vector<Waypoint> getPath() const;

  const AttachmentPoint* getAttachmentPointByNormal(Vector2 normal) const;

  // --- New Pathfinding Methods ---
  
  // For Facilities: Get a random spot index
  int getRandomSpotIndex() const;

  // For Facilities: Get the Spot data by index
  Spot getSpot(int index) const;

  // Set the state of a spot
  void setSpotState(int index, SpotState state);

  struct SpotCounts {
      int free;
      int reserved;
      int occupied;
  };
  SpotCounts getSpotCounts() const;

  // Get occupancy percentage (0.0 to 1.0)
  float getOccupancyPercentage() const;
  
  // Helper to determine facility orientation
  virtual bool isUp() const { return false; }
  
  // Accessor for local waypoints (needed by PathPlanner)
  const std::vector<Waypoint>& getLocalWaypoints() const { return localWaypoints; }

  // Type Identification
  virtual ModuleType getType() const { return ModuleType::GENERIC; }

protected:
  float width;
  float height;
  float priceMultiplier = 1.0f; // NEW
  std::vector<AttachmentPoint> attachmentPoints;
  std::vector<Waypoint> localWaypoints; // Stored relative to module top-left
  std::vector<Spot> spots; // Parking/Charging spots
  Module* parent = nullptr;
};

// --- Roads ---

class NormalRoad : public Module {
public:
  NormalRoad();
  void draw() const override;
};

class UpEntranceRoad : public Module {
public:
  UpEntranceRoad();
  void draw() const override;

};

class DownEntranceRoad : public Module {
public:
  DownEntranceRoad();
  void draw() const override;

};

class DoubleEntranceRoad : public Module {
public:
  DoubleEntranceRoad();
  void draw() const override;

};

// --- Facilities ---

class SmallParking : public Module {
public:
  SmallParking(bool isTop);
  void draw() const override;
  bool isUp() const override { return isTop; }
  ModuleType getType() const override { return ModuleType::SMALL_PARKING; }
private:
  bool isTop;
};

class LargeParking : public Module {
public:
  LargeParking(bool isTop);
  void draw() const override;
  bool isUp() const override { return isTop; }
  ModuleType getType() const override { return ModuleType::LARGE_PARKING; }
private:
  bool isTop;
};

class SmallChargingStation : public Module {
public:
  SmallChargingStation(bool isTop);
  void draw() const override;
  bool isUp() const override { return isTop; }
  ModuleType getType() const override { return ModuleType::SMALL_CHARGING; }
private:
  bool isTop;
};

class LargeChargingStation : public Module {
public:
  LargeChargingStation(bool isTop);
  void draw() const override;
  bool isUp() const override { return isTop; }
  ModuleType getType() const override { return ModuleType::LARGE_CHARGING; }
private:
  bool isTop;
};
