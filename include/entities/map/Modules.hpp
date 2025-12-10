#pragma once
#include "entities/map/Waypoint.hpp"
#include "raylib.h"
#include <vector>

struct AttachmentPoint {
  Vector2 position; // Relative to module top-left
  Vector2 normal;   // Direction of attachment
};

enum class Lane { UP, DOWN };

struct Spot {
    Vector2 localPosition;
    float orientation; // 0=Right, PI/2=Down, PI=Left, 3PI/2=Up (in radians, standard math)
                       // Or use strict: 0=Right, 90=Down, 180=Left, 270=Up?
                       // User said assets are facing UP. Rot 0 is RIGHT.
                       // Let's use standard radians for steering behavior.
    int id;
};

class Module {
public:
  Module(float w, float h) : width(w), height(h) {}
  virtual ~Module() = default;

  float getWidth() const { return width; }
  float getHeight() const { return height; }

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
  
  // Helper to determine facility orientation
  virtual bool isUp() const { return false; }
  
  // Accessor for local waypoints (needed by PathPlanner)
  const std::vector<Waypoint>& getLocalWaypoints() const { return localWaypoints; }

protected:
  float width;
  float height;
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
private:
  bool isTop;
};

class LargeParking : public Module {
public:
  LargeParking(bool isTop);
  void draw() const override;
  bool isUp() const override { return isTop; }
private:
  bool isTop;
};

class SmallChargingStation : public Module {
public:
  SmallChargingStation(bool isTop);
  void draw() const override;
  bool isUp() const override { return isTop; }
private:
  bool isTop;
};

class LargeChargingStation : public Module {
public:
  LargeChargingStation(bool isTop);
  void draw() const override;
  bool isUp() const override { return isTop; }
private:
  bool isTop;
};
