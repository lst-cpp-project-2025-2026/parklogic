#include "entities/map/Modules.hpp"
#include "config.hpp"
#include "core/AssetManager.hpp"
#include "raylib.h"
#include "raymath.h"

// --- Helper Conversion ---

static float P2M(float artPixels) {
  // ART_PIXELS_PER_METER = 7
  // So if input is art pixels, we just divide by 7.
  return artPixels / static_cast<float>(Config::ART_PIXELS_PER_METER);
}

// --- Module Base Class ---

Module::Module(float w, float h) : width(w), height(h) {
  // Base random multiplier for this facility (1.0 to 3.0)
  // This makes some facilities "posh" and others "cheap"
  priceMultiplier = (float)GetRandomValue(10, 30) / 10.0f;
}

void Module::assignRandomPricesToSpots(float baseSpotPrice, float variance) {
  for (auto &spot : spots) {
    // Spot Price = Base * FacilityMultiplier + RandomVariance
    float r = (float)GetRandomValue(-(int)(variance * 10), (int)(variance * 10)) / 10.0f;
    spot.price = (baseSpotPrice * priceMultiplier) + r;
    if (spot.price < 0.5f)
      spot.price = 0.5f; // Min price
  }
}

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

// --- New Pathfinding Implementation ---
// Logic moved to PathPlanner system.

int Module::getRandomSpotIndex() const {
  std::vector<int> freeIndices;
  for (int i = 0; i < (int)spots.size(); ++i) {
    if (spots[i].state == SpotState::FREE) {
      freeIndices.push_back(i);
    }
  }

  if (freeIndices.empty())
    return -1;

  int randIdx = GetRandomValue(0, (int)freeIndices.size() - 1);
  return freeIndices[randIdx];
}

Spot Module::getSpot(int index) const {
  if (index >= 0 && index < (int)spots.size()) {
    return spots[index];
  }
  return {{0, 0}, 0, -1, SpotState::FREE}; // Safe default
}

void Module::setSpotState(int index, SpotState state) {
  if (index >= 0 && index < (int)spots.size()) {
    spots[index].state = state;
  }
}

Module::SpotCounts Module::getSpotCounts() const {
  SpotCounts counts = {0, 0, 0};
  for (const auto &spot : spots) {
    if (spot.state == SpotState::FREE)
      counts.free++;
    else if (spot.state == SpotState::RESERVED)
      counts.reserved++;
    else if (spot.state == SpotState::OCCUPIED)
      counts.occupied++;
  }
  return counts;
}

float Module::getOccupancyPercentage() const {
  if (spots.empty())
    return 0.0f;

  int occupiedCount = 0;
  for (const auto &spot : spots) {
    if (spot.state == SpotState::OCCUPIED) {
      occupiedCount++;
    }
  }
  return (float)occupiedCount / (float)spots.size();
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

  attachmentPoints.push_back({{0, yCenter}, {-1, 0}});    // Left
  attachmentPoints.push_back({{width, yCenter}, {1, 0}}); // Right
  attachmentPoints.push_back({{xCenter, 0}, {0, -1}});    // Up

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

  attachmentPoints.push_back({{0, yCenter}, {-1, 0}});     // Left
  attachmentPoints.push_back({{width, yCenter}, {1, 0}});  // Right
  attachmentPoints.push_back({{xCenter, height}, {0, 1}}); // Down

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

  attachmentPoints.push_back({{0, yCenter}, {-1, 0}});     // Left
  attachmentPoints.push_back({{width, yCenter}, {1, 0}});  // Right
  attachmentPoints.push_back({{xCenter, 0}, {0, -1}});     // Up
  attachmentPoints.push_back({{xCenter, height}, {0, 1}}); // Down

  addWaypoint({xCenter, yCenter});
}

void DoubleEntranceRoad::draw() const {
  Texture2D tex = AssetManager::Get().GetTexture("entrance_double");
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}

// (Removed getEntryWaypoint implementation)

// --- Facilities ---

/*
small parking up : 218 330 (274*330)
small parking down : 218 0 (274*330)
*/

SmallParking::SmallParking(bool isTop) : Module(P2M(274), P2M(330)), isTop(isTop) {
  if (isTop) {
    attachmentPoints.push_back({{P2M(218), height}, {0, 1}});

    // Small Parking UP
    // 5 spots Left oriented (x=37)
    // Ys: 236, 199, 163, 127, 91
    float xLeft = P2M(37);
    float ysLeft[] = {236, 199, 163, 127, 91};
    for (float y : ysLeft)
      spots.push_back({{xLeft, P2M(y)}, PI, 0}); // Angle PI = LEFT

    // 5 spots Up oriented (y=38)
    // Xs: 90, 126, 162, 198, 234
    float yUp = P2M(38);
    float xsUp[] = {90, 126, 162, 198, 234};
    for (float x : xsUp)
      spots.push_back({{P2M(x), yUp}, 3 * PI / 2, 0}); // Angle 3PI/2 = UP

  } else {
    attachmentPoints.push_back({{P2M(218), 0}, {0, -1}});

    // Small Parking DOWN
    // 5 spots Left oriented (x=37)
    // Ys: 94, 131, 167, 203, 239
    float xLeft = P2M(37);
    float ysLeft[] = {94, 131, 167, 203, 239};
    for (float y : ysLeft)
      spots.push_back({{xLeft, P2M(y)}, PI, 0}); // Angle PI = LEFT

    // 5 spots Down oriented (y=292)
    // Xs: 90, 126, 162, 198, 234
    float yDown = P2M(292);
    float xsDown[] = {90, 126, 162, 198, 234};
    for (float x : xsDown)
      spots.push_back({{P2M(x), yDown}, PI / 2, 0}); // Angle PI/2 = DOWN
  }
  addWaypoint({P2M(218), height / 2.0f});

  // Base Price: $2.0, Variance $0.5
  assignRandomPricesToSpots(2.0f, 0.5f);
}

void SmallParking::draw() const {
  const char *texName = isTop ? "parking_small_up" : "parking_small_down";
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

    // Large Parking UP
    // 6 Left (x=38)
    float xLeft = P2M(38);
    float ysLeft[] = {269, 233, 197, 161, 125, 89};
    for (float y : ysLeft)
      spots.push_back({{xLeft, P2M(y)}, PI, 0});

    // 6 Right (x=389)
    float xRight = P2M(389);
    // Same Ys as left
    for (float y : ysLeft)
      spots.push_back({{xRight, P2M(y)}, 0.0f, 0}); // Angle 0 = RIGHT

    // 8 Up (y=38)
    float yUp = P2M(38);
    float xsUp[] = {92, 128, 164, 200, 236, 272, 308, 344};
    for (float x : xsUp)
      spots.push_back({{P2M(x), yUp}, 3 * PI / 2, 0});

  } else {
    attachmentPoints.push_back({{P2M(218), 0}, {0, -1}});

    // Large Parking DOWN
    // 6 Left (x=38)
    float xLeft = P2M(38);
    float ysLeft[] = {94, 130, 166, 202, 238, 274};
    for (float y : ysLeft)
      spots.push_back({{xLeft, P2M(y)}, PI, 0});

    // 6 Right (x=389)
    float xRight = P2M(389);
    for (float y : ysLeft)
      spots.push_back({{xRight, P2M(y)}, 0.0f, 0});

    // 8 Down (y=325)
    float yDown = P2M(325);
    float xsDown[] = {92, 128, 164, 200, 236, 272, 308, 344};
    for (float x : xsDown)
      spots.push_back({{P2M(x), yDown}, PI / 2, 0});
  }
  addWaypoint({P2M(218), height / 2.0f});

  // Base Price: $1.0, Variance $0.5
  assignRandomPricesToSpots(1.0f, 0.5f);
}

void LargeParking::draw() const {
  const char *texName = isTop ? "parking_large_up" : "parking_large_down";
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

    // Small Charging UP
    // 5 Up (y=38)
    float yUp = P2M(38);
    float xsUp[] = {38, 73, 109, 145, 181};
    for (float x : xsUp)
      spots.push_back({{P2M(x), yUp}, 3 * PI / 2, 0});

  } else {
    attachmentPoints.push_back({{P2M(163), 0}, {0, -1}});

    // Small Charging DOWN
    // 5 Down (y=130)
    float yDown = P2M(130);
    float xsDown[] = {38, 73, 109, 145, 181};
    for (float x : xsDown)
      spots.push_back({{P2M(x), yDown}, PI / 2, 0});
  }
  if (isTop) {
    // Entrance at Bottom (Height)
    // Original Center: Height/2
    // Fix: Move closer to entrance (Height * 0.8)
    addWaypoint({P2M(163), height * 0.85f});
  } else {
    // Entrance at Top (0)
    // Fix: Move closer to entrance (Height * 0.2)
    addWaypoint({P2M(163), height * 0.15f});
  }

  // Charging is more expensive (2-5x more than parking)
  // Base Price: $10.0, Variance $1.0
  // priceMultiplier *= 1.5f; // Add extra multiplier boost for being a charging station module?
  assignRandomPricesToSpots(10.0f, 1.0f);
}

void SmallChargingStation::draw() const {
  const char *texName = isTop ? "charging_small_up" : "charging_small_down";
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
    // Same layout as Small Parking UP
    float xLeft = P2M(37);
    float ysLeft[] = {236, 199, 163, 127, 91};
    for (float y : ysLeft)
      spots.push_back({{xLeft, P2M(y)}, PI, 0});

    float yUp = P2M(38);
    float xsUp[] = {90, 126, 162, 198, 234};
    for (float x : xsUp)
      spots.push_back({{P2M(x), yUp}, 3 * PI / 2, 0});

  } else {
    attachmentPoints.push_back({{P2M(218), 0}, {0, -1}});
    // Same layout as Small Parking DOWN
    float xLeft = P2M(37);
    float ysLeft[] = {94, 131, 167, 203, 239};
    for (float y : ysLeft)
      spots.push_back({{xLeft, P2M(y)}, PI, 0});

    float yDown = P2M(292);
    float xsDown[] = {90, 126, 162, 198, 234};
    for (float x : xsDown)
      spots.push_back({{P2M(x), yDown}, PI / 2, 0});
  }
  addWaypoint({P2M(218), height / 2.0f});

  // Base Price: $8.0, Variance $2.0
  priceMultiplier *= 1.5f;
  assignRandomPricesToSpots(8.0f, 2.0f);
}

void LargeChargingStation::draw() const {
  const char *texName = isTop ? "charging_large_up" : "charging_large_down";
  Texture2D tex = AssetManager::Get().GetTexture(texName);
  Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dest = {worldPosition.x, worldPosition.y, width, height};
  DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
  Module::draw();
}
