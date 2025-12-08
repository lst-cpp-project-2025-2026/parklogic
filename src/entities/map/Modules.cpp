#include "entities/map/Modules.hpp"
#include "config.hpp"
#include "core/AssetManager.hpp"
#include "raylib.h"
#include "raymath.h"
#include <iostream>

// --- Helper Conversion ---

static float P2M(float artPixels) {
  // 3 resolution pixels per art pixel
  // 7 art pixels per meter
  // So 21 resolution pixels per meter...
  //
  // But wait, the config says:
  // ART_PIXELS_PER_METER = 7
  // So if input is art pixels, we just divide by 7.
  return artPixels / static_cast<float>(Config::ART_PIXELS_PER_METER);
}

// --- Module Base Class ---

void Module::draw() const {
  // Default draw: outline (in Meters)
  // DrawRectangleLinesEx({worldPosition.x, worldPosition.y, width, height}, 0.1f, BLACK);

  // Draw Waypoints (Debug)
  // for (const auto &lwp : localWaypoints) {
  //   Vector2 globalPos = Vector2Add(worldPosition, lwp.position);
  //   DrawCircleV(globalPos, 0.2f, Fade(ORANGE, 0.6f));
  // }
}

void Module::addWaypoint(Vector2 localPos, float tolerance, int id, float angle, bool stop) {
  localWaypoints.emplace_back(localPos, tolerance, id, angle, stop);
}

std::vector<Waypoint> Module::getGlobalWaypoints() const {
  std::vector<Waypoint> globalWps;
  for (const auto &lwp : localWaypoints) {
    globalWps.emplace_back(Vector2Add(worldPosition, lwp.position), lwp.tolerance, lwp.id, lwp.entryAngle,
                           lwp.stopAtEnd);
  }
  return globalWps;
}

std::vector<Waypoint> Module::getPath() const {
  std::vector<Waypoint> path;

  // 1. Get Parent's path first (Recursive)
  if (parent) {
    path = parent->getPath();
  }

  // 2. Append my own waypoints
  std::vector<Waypoint> myWps = getGlobalWaypoints();
  path.insert(path.end(), myWps.begin(), myWps.end());

  return path;
}

const AttachmentPoint *Module::getAttachmentPointByNormal(Vector2 normal) const {
  for (const auto &ap : attachmentPoints) {
    // Approx comparison
    if (Vector2Distance(ap.normal, normal) < 0.1f) {
      return &ap;
    }
  }
  return nullptr;
}

// --- Roads ---
// normal road : left (0 78) right (283 78) size (283 155)

NormalRoad::NormalRoad() : Module(P2M(283), P2M(155)) {
  // Left: 0, 78 (art pixels)
  // Right: 283, 78
  // Y in meters = 78 / 7 = 11.14
  
  float yCenter = P2M(78);

  // Left (-1, 0)
  attachmentPoints.push_back({{0, yCenter}, {-1, 0}});
  // Right (1, 0)
  attachmentPoints.push_back({{width, yCenter}, {1, 0}});

  // Waypoints: Center
  addWaypoint({width / 2.0f, yCenter});
}

void NormalRoad::draw() const {
  Texture2D tex = AssetManager::Get().GetTexture("road");
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  // DrawTexturePro destination uses width/height in world units
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  
  Module::draw();
}

// up entrance road : left (0 78) right (283 78) up(142 0) size (284 155)
UpEntranceRoad::UpEntranceRoad() : Module(P2M(284), P2M(155)) {
    float yCenter = P2M(78);
    float xCenter = P2M(142);

  attachmentPoints.push_back({{0, yCenter}, {-1, 0}});       // Left
  attachmentPoints.push_back({{width, yCenter}, {1, 0}});    // Right
  attachmentPoints.push_back({{xCenter, 0}, {0, -1}});       // Up

  addWaypoint({xCenter, yCenter});
}

void UpEntranceRoad::draw() const {
  Texture2D tex = AssetManager::Get().GetTexture("entrance_up");
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}

// down entrance road : left (0 78) right (283 78) down(142 155) size (284 155)
DownEntranceRoad::DownEntranceRoad() : Module(P2M(284), P2M(155)) {
    float yCenter = P2M(78);
    float xCenter = P2M(142);

  attachmentPoints.push_back({{0, yCenter}, {-1, 0}});       // Left
  attachmentPoints.push_back({{width, yCenter}, {1, 0}});    // Right
  attachmentPoints.push_back({{xCenter, height}, {0, 1}});   // Down

  addWaypoint({xCenter, yCenter});
}

void DownEntranceRoad::draw() const {
  Texture2D tex = AssetManager::Get().GetTexture("entrance_down");
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}

// double entrance road : left (0 78) right (283 78) up(142 0) down(142 155) size (284 155)
DoubleEntranceRoad::DoubleEntranceRoad() : Module(P2M(284), P2M(155)) {
    float yCenter = P2M(78);
    float xCenter = P2M(142);

  attachmentPoints.push_back({{0, yCenter}, {-1, 0}});       // Left
  attachmentPoints.push_back({{width, yCenter}, {1, 0}});    // Right
  attachmentPoints.push_back({{xCenter, 0}, {0, -1}});       // Up
  attachmentPoints.push_back({{xCenter, height}, {0, 1}});   // Down

  addWaypoint({xCenter, yCenter});
}

void DoubleEntranceRoad::draw() const {
  Texture2D tex = AssetManager::Get().GetTexture("entrance_double");
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}

// --- Facilities ---

/*
small parking up : 218 330 (274*330)
small parking down : 218 0 (274*330)
*/

SmallParking::SmallParking(bool isTop) : Module(P2M(274), P2M(330)), isTop(isTop) {
  if (isTop) {
      // Connects to an Up entrance (so its entrance handles "Down" normal from itself)
      // "up" (for those below means that they will connect to an up (or double) entrance, ie their entrance is pointing down)
      // Point: 218 330
      // Attachment Normal: (0, 1) - pointing down
      attachmentPoints.push_back({{P2M(218), height}, {0, 1}});
  } else {
      // Connects to a Down entrance (so its entrance handles "Up" normal from itself)
      // Point: 218 0
      // Attachment Normal: (0, -1) - pointing up
      attachmentPoints.push_back({{P2M(218), 0}, {0, -1}});
  }
  addWaypoint({P2M(218), height / 2.0f});
}

void SmallParking::draw() const {
  const char* texName = isTop ? "parking_small_up" : "parking_small_down";
  Texture2D tex = AssetManager::Get().GetTexture(texName);
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}

/*
large parking up : 218 363 (436*363)
large parking down : 218 0 (436*363)
*/
LargeParking::LargeParking(bool isTop) : Module(P2M(436), P2M(363)), isTop(isTop) {
  if (isTop) {
      attachmentPoints.push_back({{P2M(218), height}, {0, 1}});
  } else {
      attachmentPoints.push_back({{P2M(218), 0}, {0, -1}});
  }
  addWaypoint({P2M(218), height / 2.0f});
}

void LargeParking::draw() const {
  const char* texName = isTop ? "parking_large_up" : "parking_large_down";
  Texture2D tex = AssetManager::Get().GetTexture(texName);
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}


/*
small charging up : 163 168 (219*168)
small charging down : 163 0 (219*168)
*/
SmallChargingStation::SmallChargingStation(bool isTop) : Module(P2M(219), P2M(168)), isTop(isTop) {
   if (isTop) {
      attachmentPoints.push_back({{P2M(163), height}, {0, 1}});
  } else {
      attachmentPoints.push_back({{P2M(163), 0}, {0, -1}});
  }
  addWaypoint({P2M(163), height / 2.0f});
}

void SmallChargingStation::draw() const {
  const char* texName = isTop ? "charging_small_up" : "charging_small_down";
  Texture2D tex = AssetManager::Get().GetTexture(texName);
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}

/*
large charging up : 218 330 (274*330)
large charging down : 218 0 (274*330)
*/
LargeChargingStation::LargeChargingStation(bool isTop) : Module(P2M(274), P2M(330)), isTop(isTop) {
   if (isTop) {
      attachmentPoints.push_back({{P2M(218), height}, {0, 1}});
  } else {
      attachmentPoints.push_back({{P2M(218), 0}, {0, -1}});
  }
  addWaypoint({P2M(218), height / 2.0f});
}

void LargeChargingStation::draw() const {
  const char* texName = isTop ? "charging_large_up" : "charging_large_down";
  Texture2D tex = AssetManager::Get().GetTexture(texName);
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}
