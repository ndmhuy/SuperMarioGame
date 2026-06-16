#include "Utils/TileMap.hpp"

void TileMap::render(sf::RenderTarget& target, Camera& camera) {
    // TODO: Implement by hand
}

int TileMap::getTileAt(float px, float py) const {
    // TODO: Implement by hand
    return 0;
}

sf::Vector2i TileMap::worldToGrid(float px, float py) const {
    // TODO: Implement by hand
    return sf::Vector2i{0, 0};
}

sf::Vector2f TileMap::gridToWorld(int gx, int gy) const {
    // TODO: Implement by hand
    return sf::Vector2f{0.0f, 0.0f};
}

int TileMap::getTileSurfaceType(float px, float py) const {
    // TODO: Implement by hand
    return 0;
}

void TileMap::swapBricksAndCoins() {
    // TODO: Implement by hand
}

void TileMap::initialize(int width, int height) {
    m_width = width;
    m_height = height;
    m_grid.assign(height, std::vector<int>(width, 0));
}

void TileMap::setTile(int gx, int gy, int type) {
    if (gx >= 0 && gx < m_width && gy >= 0 && gy < m_height) {
        m_grid[gy][gx] = type;
    }
}
