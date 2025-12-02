#pragma once
#include "raylib.h"
#include <map>
#include <string>

class AssetManager {
public:
  static AssetManager &Get() {
    static AssetManager instance;
    return instance;
  }

  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  // Textures
  void LoadTexture(const std::string &name, const std::string &path);
  Texture2D GetTexture(const std::string &name);
  void UnloadTexture(const std::string &name);

  // General
  void UnloadAll();

private:
  AssetManager() = default;
  ~AssetManager();

  std::map<std::string, Texture2D> textures;
  std::map<std::string, Sound> sounds;
};
