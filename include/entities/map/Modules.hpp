#pragma once
#include "entities/map/Waypoint.hpp"
#include "raylib.h"
#include <vector>

struct AttachmentPoint {
  Vector2 position; // Relative to module top-left
  Vector2 normal;   // Direction of attachment
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

  // Recursive path retrieval
  std::vector<Waypoint> getPath() const;

  const AttachmentPoint* getAttachmentPointByNormal(Vector2 normal) const;

protected:
  float width;
  float height;
  std::vector<AttachmentPoint> attachmentPoints;
  std::vector<Waypoint> localWaypoints; // Stored relative to module top-left
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
private:
  bool isTop;
};

class LargeParking : public Module {
public:
  LargeParking(bool isTop);
  void draw() const override;
private:
  bool isTop;
};

class SmallChargingStation : public Module {
public:
  SmallChargingStation(bool isTop);
  void draw() const override;
private:
  bool isTop;
};

class LargeChargingStation : public Module {
public:
  LargeChargingStation(bool isTop);
  void draw() const override;
private:
  bool isTop;
};
