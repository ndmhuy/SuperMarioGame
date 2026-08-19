#include "Core/ResourceManager.hpp"
#include <iostream>
#include <filesystem>

ResourceManager &ResourceManager::getInstance() {
  static ResourceManager instance;
  return instance;
}

std::string ResourceManager::resolvePath(const std::string &relativePath) {
  if (relativePath.empty()) return relativePath;
  if (std::filesystem::exists(relativePath)) return relativePath;
  if (std::filesystem::exists("../" + relativePath)) return "../" + relativePath;
  if (std::filesystem::exists("../../" + relativePath)) return "../../" + relativePath;
  if (std::filesystem::exists("SuperMarioGame/" + relativePath)) return "SuperMarioGame/" + relativePath;
  if (std::filesystem::exists("../SuperMarioGame/" + relativePath)) return "../SuperMarioGame/" + relativePath;
  if (std::filesystem::exists("../../SuperMarioGame/" + relativePath)) return "../../SuperMarioGame/" + relativePath;
  return relativePath;
}

bool ResourceManager::loadTexture(const std::string &id,
                                  const std::string &path) {
  std::string actualPath = resolvePath(path);
  sf::Texture texture;
  if (texture.loadFromFile(actualPath)) {
    m_textures[id] = std::move(texture);
    return true;
  }
  std::cerr << "[ResourceManager] Failed to load texture: " << actualPath
            << std::endl;
  return false;
}

bool ResourceManager::hasTexture(const std::string &id) const {
  return m_textures.find(id) != m_textures.end();
}

bool ResourceManager::hasFont(const std::string &id) const {
  return m_fonts.find(id) != m_fonts.end();
}

bool ResourceManager::hasSoundBuffer(const std::string &id) const {
  return m_soundBuffers.find(id) != m_soundBuffers.end();
}

sf::Texture &ResourceManager::getTexture(const std::string &id) {
  auto it = m_textures.find(id);
  if (it != m_textures.end()) {
    return it->second;
  }
  if (m_reportedMissing.find("texture:" + id) == m_reportedMissing.end()) {
    std::cerr << "[ResourceManager] Texture not found: " << id << std::endl;
    m_reportedMissing.insert("texture:" + id);
  }
  // The placeholder lives in the map, not in a function-local static. A static
  // sf::Texture is destroyed *after* the graphics context, which aborts the
  // process during static destruction; keeping it here means clear() releases it
  // while the context is still alive.
  return m_textures[id];
}

bool ResourceManager::loadFont(const std::string &id, const std::string &path) {
  std::string actualPath = resolvePath(path);
  sf::Font font;
  if (font.openFromFile(actualPath)) {
    m_fonts[id] = std::move(font);
    return true;
  }
  if (m_reportedMissing.find("font_path:" + actualPath) == m_reportedMissing.end()) {
    std::cerr << "[ResourceManager] Failed to load font: " << actualPath << std::endl;
    m_reportedMissing.insert("font_path:" + actualPath);
  }
  return false;
}

sf::Font &ResourceManager::getFont(const std::string &id) {
  auto it = m_fonts.find(id);
  if (it != m_fonts.end()) {
    return it->second;
  }
  if (m_reportedMissing.find("font:" + id) == m_reportedMissing.end()) {
    std::cerr << "[ResourceManager] Font not found: " << id << std::endl;
    m_reportedMissing.insert("font:" + id);
  }
  // Placeholder kept in the map for the same reason as getTexture(): sf::Font
  // owns a glyph texture, and a function-local static one outlives the context.
  return m_fonts[id];
}

bool ResourceManager::loadSoundBuffer(const std::string &id,
                                      const std::string &path) {
  std::string actualPath = resolvePath(path);
  sf::SoundBuffer soundBuffer;
  if (soundBuffer.loadFromFile(actualPath)) {
    m_soundBuffers[id] = std::move(soundBuffer);
    return true;
  }
  if (m_reportedMissing.find("sfx_path:" + actualPath) == m_reportedMissing.end()) {
    std::cerr << "[ResourceManager] Failed to load sound buffer: " << actualPath << std::endl;
    m_reportedMissing.insert("sfx_path:" + actualPath);
  }
  return false;
}

sf::SoundBuffer &ResourceManager::getSoundBuffer(const std::string &id) {
  auto it = m_soundBuffers.find(id);
  if (it != m_soundBuffers.end()) {
    return it->second;
  }
  if (m_reportedMissing.find("sound:" + id) == m_reportedMissing.end()) {
    std::cerr << "[ResourceManager] SoundBuffer not found: " << id << std::endl;
    m_reportedMissing.insert("sound:" + id);
  }
  // Placeholder kept in the map, so shutdown order stays under our control.
  return m_soundBuffers[id];
}

void ResourceManager::clear() {
  m_textures.clear();
  m_fonts.clear();
  m_soundBuffers.clear();
  m_reportedMissing.clear();
}
