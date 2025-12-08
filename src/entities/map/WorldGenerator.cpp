#include "entities/map/WorldGenerator.hpp"
#include "core/Logger.hpp"
#include "entities/map/Modules.hpp"
#include "raymath.h"
#include <algorithm>
#include <random>
#include <vector>

struct PlannedUnit {
  std::unique_ptr<Module> road;
  std::unique_ptr<Module> topFacility;
  std::unique_ptr<Module> bottomFacility;
};

GeneratedMap WorldGenerator::generate(const MapConfig& config) {
  Logger::Info("Generating World...");

  std::vector<std::unique_ptr<Module>> modules;
  std::vector<PlannedUnit> plan;

  std::random_device rd;
  std::mt19937 gen(rd());

  // Configuration
  std::uniform_int_distribution<> roadTypeDist(0, 2);     // 0: Up, 1: Down, 2: Double
  
  int smallParkingLeft = config.smallParkingCount;
  int largeParkingLeft = config.largeParkingCount;
  int smallChargingLeft = config.smallChargingCount;
  int largeChargingLeft = config.largeChargingCount;

  // CreateFacility needs to know if it is for Top (connects to Up entrance) -> isTop=true
  // Or Bottom (connects to Down entrance) -> isTop=false
  auto createFacility = [&](int type, int size, bool isTop) -> std::unique_ptr<Module> {
    if (type == 0) { // Parking
      if (size == 0)
        return std::make_unique<SmallParking>(isTop);
      else
        return std::make_unique<LargeParking>(isTop);
    } else { // Charging
      if (size == 0)
        return std::make_unique<SmallChargingStation>(isTop);
      else
        return std::make_unique<LargeChargingStation>(isTop);
    }
  };

  while (smallParkingLeft > 0 || largeParkingLeft > 0 || smallChargingLeft > 0 || largeChargingLeft > 0) {
    PlannedUnit unit;
    int rType = roadTypeDist(gen);

    // Helper to get next facility type
    auto getNextFacility = [&](bool isTop) -> std::unique_ptr<Module> {
        std::vector<int> availableTypes;
        if (smallParkingLeft > 0) availableTypes.push_back(0); // 0: Small Parking
        if (largeParkingLeft > 0) availableTypes.push_back(1); // 1: Large Parking
        if (smallChargingLeft > 0) availableTypes.push_back(2); // 2: Small Charging
        if (largeChargingLeft > 0) availableTypes.push_back(3); // 3: Large Charging

        if (availableTypes.empty()) return nullptr;

        std::uniform_int_distribution<> dist(0, availableTypes.size() - 1);
        int choice = availableTypes[dist(gen)];

        if (choice == 0) { smallParkingLeft--; return createFacility(0, 0, isTop); }
        if (choice == 1) { largeParkingLeft--; return createFacility(0, 1, isTop); }
        if (choice == 2) { smallChargingLeft--; return createFacility(1, 0, isTop); }
        if (choice == 3) { largeChargingLeft--; return createFacility(1, 1, isTop); }
        
        return nullptr;
    };

    if (rType == 0) { // Up
      unit.road = std::make_unique<UpEntranceRoad>();
      unit.topFacility = getNextFacility(true);
    } else if (rType == 1) { // Down
      unit.road = std::make_unique<DownEntranceRoad>();
      unit.bottomFacility = getNextFacility(false);
    } else { // Double
      unit.road = std::make_unique<DoubleEntranceRoad>();
      unit.topFacility = getNextFacility(true);
      unit.bottomFacility = getNextFacility(false);
    }

    // Link Facilities to Road (Parenting)
    if (unit.topFacility) {
        unit.topFacility->setParent(unit.road.get());
    }
    if (unit.bottomFacility) {
        unit.bottomFacility->setParent(unit.road.get());
    }
    
    plan.push_back(std::move(unit));
  }

  // Placement Logic
  float currentX = 0.0f;
  float startY = 50.0f;   // Middle of map (arbitrary start)
  float safeX = -1000.0f; // Rightmost edge of occupied space (for collision prevention)
  
  // Roads are different widths now (mostly 283 or 284 art pixels ~ 40 meters)
  
  // Helper to place a single standard road
  auto placePaddingRoad = [&](float &x, float y) {
      auto road = std::make_unique<NormalRoad>();
      // Road origin is top-left.
      // We align based on Left attachment.
      // NormalRoad attaches Left at (0, yCenter)
      // So if currentX is the connection point, we place road such that Left attach is at currentX.
      // The road's position will simply be (x, y - yCenterOffset).
      // But standard linear placement:
      // Road.yCenter ~ 11.14m. 
      // Let's assume the "spine" of the road is at global Y = startY.
      // Road.worldY = startY - road.LeftAttach.y
      
      const auto* leftAttach = road->getAttachmentPointByNormal({-1, 0});
      if (leftAttach) {
          road->worldPosition = {x - leftAttach->position.x, y - leftAttach->position.y};
          
          const auto* rightAttach = road->getAttachmentPointByNormal({1, 0});
          if (rightAttach) {
              x += rightAttach->position.x; // Advance X by width (effectively)
          } else {
              x += road->getWidth();
          }
      }
      modules.push_back(std::move(road));
  };
  
    // Add initial padding roads
    for (int i = 0; i < 3; ++i) {
        placePaddingRoad(currentX, startY);
    }
    safeX = currentX;

  for (auto &unit : plan) {
    if (!unit.road) continue;

    // 1. Calculate Overhangs from Facilities
    float leftMinX = currentX;
    float rightMaxX = currentX;
    
    // We haven't placed the road yet, but let's simulate where it would be.
    // Road attaches at currentX.
    // Road WorldPos = ...
    const auto* roadLeft = unit.road->getAttachmentPointByNormal({-1, 0});
    if (!roadLeft) {
        Logger::Error("Road missing left attachment point!");
        continue;
    }
    
    Vector2 roadWorldPos = {currentX - roadLeft->position.x, startY - roadLeft->position.y};
    
    float roadRightX = roadWorldPos.x + unit.road->getWidth();
    if (const auto* rRight = unit.road->getAttachmentPointByNormal({1, 0})) {
        roadRightX = roadWorldPos.x + rRight->position.x;
    }

    rightMaxX = std::max(rightMaxX, roadRightX);

    // Lambda to check facility extents
    auto checkFacility = [&](const std::unique_ptr<Module> &fac, Vector2 roadAttachNormal) {
        if (!fac) return;
        
        // Road has attachment pointing 'roadAttachNormal' (e.g. Up: {0, -1})
        const auto* roadAttach = unit.road->getAttachmentPointByNormal(roadAttachNormal);
        if (!roadAttach) return;
        
        // Facility connects with opposite normal
        Vector2 facNormal = Vector2Scale(roadAttachNormal, -1.0f);
        const auto* facAttach = fac->getAttachmentPointByNormal(facNormal);
        if (!facAttach) return;
        
        // Calculated World Pos of Facility
        Vector2 globalAttachEx = Vector2Add(roadWorldPos, roadAttach->position); // Where connection happens in world
        
        // Fac.WorldPos + FacAttach.Pos = GlobalAttachEx
        // Fac.WorldPos = GlobalAttachEx - FacAttach.Pos
        Vector2 facWorldPos = Vector2Subtract(globalAttachEx, facAttach->position);
        
        // Update Extents
        if (facWorldPos.x < leftMinX) leftMinX = facWorldPos.x;
        if (facWorldPos.x + fac->getWidth() > rightMaxX) rightMaxX = facWorldPos.x + fac->getWidth();
    };

    checkFacility(unit.topFacility, {0, -1});    // Road Up
    checkFacility(unit.bottomFacility, {0, 1});  // Road Down
    
    // 2. Ensure Space (Collision with previous modules)
    // safeX is the rightmost X of the previous block
    // We need leftMinX >= safeX
    // If leftMinX < safeX, we need to shift everything right.
    // The "shift" is achieved by adding padding roads.
    
    // While our *projected* left edge overlaps with safe zone...
    while (leftMinX < safeX) {
        placePaddingRoad(currentX, startY);
        // Recalculate projection
        // roadWorldPos, leftMinX, etc. would shift by roadWidth.
        // Simplification: just increment currentX and recalculate extents relative to it.
        // Actually, just placing the road advances currentX.
        
        // Re-simulate
        roadWorldPos = {currentX - roadLeft->position.x, startY - roadLeft->position.y};
        // Reset and re-check
        leftMinX = currentX; // Road starts at currentX (visually attached)
        rightMaxX = currentX;
        
        // Re-run checks
         if (const auto* rRight = unit.road->getAttachmentPointByNormal({1, 0})) {
             rightMaxX = roadWorldPos.x + rRight->position.x;
         }

        auto reCheck = [&](const std::unique_ptr<Module> &fac, Vector2 roadAttachNormal) {
             if (!fac) return;
            const auto* roadAttach = unit.road->getAttachmentPointByNormal(roadAttachNormal);
            if (!roadAttach) return;
            Vector2 facNormal = Vector2Scale(roadAttachNormal, -1.0f);
            const auto* facAttach = fac->getAttachmentPointByNormal(facNormal);
            if (!facAttach) return;
            Vector2 globalAttachEx = Vector2Add(roadWorldPos, roadAttach->position);
            Vector2 facWorldPos = Vector2Subtract(globalAttachEx, facAttach->position);
            if (facWorldPos.x < leftMinX) leftMinX = facWorldPos.x;
            if (facWorldPos.x + fac->getWidth() > rightMaxX) rightMaxX = facWorldPos.x + fac->getWidth();
        };
        reCheck(unit.topFacility, {0, -1});
        reCheck(unit.bottomFacility, {0, 1});
    }

    // 3. Place Unit Final
    unit.road->worldPosition = roadWorldPos;
    
    auto placeFac = [&](std::unique_ptr<Module> &fac, Vector2 roadAttachNormal) {
        if (!fac) return;
        const auto* roadAttach = unit.road->getAttachmentPointByNormal(roadAttachNormal);
        if (!roadAttach) return;  // Should not happen
        Vector2 facNormal = Vector2Scale(roadAttachNormal, -1.0f);
        const auto* facAttach = fac->getAttachmentPointByNormal(facNormal);
        if (!facAttach) return; // Should not happen
        
        Vector2 globalAttachEx = Vector2Add(roadWorldPos, roadAttach->position);
        fac->worldPosition = Vector2Subtract(globalAttachEx, facAttach->position);
        
        modules.push_back(std::move(fac));
    };

    placeFac(unit.topFacility, {0, -1});
    placeFac(unit.bottomFacility, {0, 1});
    
    modules.push_back(std::move(unit.road));

    // Update safeX and currentX
    safeX = rightMaxX;
    
    // Advance currentX to the right attachment point of the placed road
    if (const auto* rRight = modules.back()->getAttachmentPointByNormal({1, 0})) {
         // The last module pushed was the road
         // currentX currently points to Left Attach Global X. 
         // We want next currentX to be Right Attach Global X.
         // Road World X + Right Attach Local X
         currentX = modules.back()->worldPosition.x + rRight->position.x;
    } else {
        currentX += modules.back()->getWidth();
    }
  }

  // Add end padding roads
  for (int i = 0; i < 3; ++i) {
    while (currentX < safeX) {
      placePaddingRoad(currentX, startY);
    }
    placePaddingRoad(currentX, startY);
    safeX = currentX; // Treat road end as safe
  }

  // Calculate World Bounds
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();

  for (const auto &mod : modules) {
    if (mod->worldPosition.x < minX)
      minX = mod->worldPosition.x;
    if (mod->worldPosition.y < minY)
      minY = mod->worldPosition.y;
    if (mod->worldPosition.x + mod->getWidth() > maxX)
      maxX = mod->worldPosition.x + mod->getWidth();
    if (mod->worldPosition.y + mod->getHeight() > maxY)
      maxY = mod->worldPosition.y + mod->getHeight();
  }

  float padding = 20.0f;
  float contentWidth = maxX - minX;
  float contentHeight = maxY - minY;

  float worldWidth = contentWidth + 2 * padding;
  float worldHeight = contentHeight + 2 * padding;

  // Shift modules to center (which means starting at padding)
  // We want minX to be at 'padding', minY to be at 'padding'
  float offsetX = padding - minX;
  float offsetY = padding - minY;

  for (auto &mod : modules) {
    mod->worldPosition.x += offsetX;
    mod->worldPosition.y += offsetY;
  }

  auto world = std::make_unique<World>(worldWidth, worldHeight);

  return {std::move(world), std::move(modules)};
}
