#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include "Utils/MathUtils.hpp"
#include "Graphics/Camera.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>
#include <algorithm>

void TileMap::render(sf::RenderTarget& target, Camera& camera) {
    AABB visible = camera.getVisibleBounds();
    int startX = static_cast<int>(std::floor(visible.x / Constants::TILE_SIZE));
    int endX = static_cast<int>(std::floor((visible.x + visible.width) / Constants::TILE_SIZE));
    int startY = static_cast<int>(std::floor(visible.y / Constants::TILE_SIZE));
    int endY = static_cast<int>(std::floor((visible.y + visible.height) / Constants::TILE_SIZE));

    startX = MathUtils::clamp(startX, 0, m_width - 1);
    endX = MathUtils::clamp(endX, 0, m_width - 1);
    startY = MathUtils::clamp(startY, 0, m_height - 1);
    endY = MathUtils::clamp(endY, 0, m_height - 1);

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            int tileType = m_grid[y][x];
            if (tileType == 0) continue;

            sf::RectangleShape rect(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
            rect.setPosition(gridToWorld(x, y));

            if (tileType == 1) {
                rect.setFillColor(sf::Color(120, 80, 30)); // Ground
            } else if (tileType == 2) {
                rect.setFillColor(sf::Color(180, 50, 50)); // Brick
            } else if (tileType == 3) {
                rect.setFillColor(sf::Color(230, 180, 30)); // Question block
            } else if (tileType == 4) {
                rect.setFillColor(sf::Color(30, 180, 30)); // Pipe
            } else if (tileType == 5) {
                rect.setFillColor(sf::Color(150, 220, 255)); // Ice
            } else if (tileType == 6) {
                rect.setFillColor(sf::Color(180, 180, 180)); // Conveyor
            } else if (tileType == 7) {
                rect.setFillColor(sf::Color(30, 100, 230, 128)); // Water
            } else if (tileType == 8) {
                rect.setFillColor(sf::Color(255, 215, 0)); // Coin
            }

            target.draw(rect);
        }
    }
}

int TileMap::getTileAt(float px, float py) const {
    sf::Vector2i gridPos = worldToGrid(px, py);
    return getTileType(gridPos.x, gridPos.y);
}

sf::Vector2i TileMap::worldToGrid(float px, float py) const {
    int gx = static_cast<int>(std::floor(px / Constants::TILE_SIZE));
    int gy = static_cast<int>(std::floor(py / Constants::TILE_SIZE));
    return sf::Vector2i{gx, gy};
}

sf::Vector2f TileMap::gridToWorld(int gx, int gy) const {
    return sf::Vector2f{gx * Constants::TILE_SIZE, gy * Constants::TILE_SIZE};
}

int TileMap::getTileSurfaceType(float px, float py) const {
    return getTileAt(px, py);
}

void TileMap::swapBricksAndCoins() {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            if (m_grid[y][x] == 2) {
                m_grid[y][x] = 8;
            } else if (m_grid[y][x] == 8) {
                m_grid[y][x] = 2;
            }
        }
    }
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
