#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include "Utils/MathUtils.hpp"
#include "Graphics/Camera.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>
#include <algorithm>

const TileInfo& TileMap::getInfo(TileType type) {
    switch (type) {
        case TileType::Empty: {
            static const TileInfo info{TileType::Empty, false, sf::Color::Transparent, "Empty"};
            return info;
        }
        case TileType::Ground: {
            static const TileInfo info{TileType::Ground, true, sf::Color(120, 80, 30), "Ground"};
            return info;
        }
        case TileType::Brick: {
            static const TileInfo info{TileType::Brick, true, sf::Color(180, 50, 50), "Brick"};
            return info;
        }
        case TileType::Question: {
            static const TileInfo info{TileType::Question, true, sf::Color(230, 180, 30), "Question"};
            return info;
        }
        case TileType::Pipe: {
            static const TileInfo info{TileType::Pipe, true, sf::Color(30, 180, 30), "Pipe"};
            return info;
        }
        case TileType::Ice: {
            static const TileInfo info{TileType::Ice, true, sf::Color(150, 220, 255), "Ice"};
            return info;
        }
        case TileType::Conveyor: {
            static const TileInfo info{TileType::Conveyor, true, sf::Color(180, 180, 180), "Conveyor"};
            return info;
        }
        case TileType::Water: {
            static const TileInfo info{TileType::Water, false, sf::Color(30, 100, 230, 128), "Water"};
            return info;
        }
        case TileType::Coin: {
            static const TileInfo info{TileType::Coin, false, sf::Color(255, 215, 0), "Coin"};
            return info;
        }
        case TileType::Used: {
            static const TileInfo info{TileType::Used, true, sf::Color(100, 70, 20), "Used"};
            return info;
        }
        default: {
            static const TileInfo defaultInfo{TileType::Empty, false, sf::Color::Transparent, "Unknown"};
            return defaultInfo;
        }
    }
}

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
            TileType tileType = m_grid[y][x];
            const TileInfo& info = getInfo(tileType);
            if (info.type == TileType::Empty) continue;

            sf::RectangleShape rect(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
            rect.setPosition(gridToWorld(x, y));
            rect.setFillColor(info.debugColor);

            target.draw(rect);
        }
    }
}

TileType TileMap::getTileAt(float px, float py) const {
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

TileType TileMap::getTileSurfaceType(float px, float py) const {
    return getTileAt(px, py);
}

void TileMap::swapBricksAndCoins() {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            if (m_grid[y][x] == TileType::Brick) {
                m_grid[y][x] = TileType::Coin;
            } else if (m_grid[y][x] == TileType::Coin) {
                m_grid[y][x] = TileType::Brick;
            }
        }
    }
}

void TileMap::initialize(int width, int height) {
    m_width = width;
    m_height = height;
    m_grid.assign(height, std::vector<TileType>(width, TileType::Empty));
}

void TileMap::expandToFit(int requiredWidth, int requiredHeight) {
    if (requiredWidth <= m_width && requiredHeight <= m_height) return;

    int newWidth = std::max(m_width, requiredWidth);
    int newHeight = std::max(m_height, requiredHeight);

    for (int y = 0; y < m_height; ++y) {
        m_grid[y].resize(newWidth, TileType::Empty);
    }
    for (int y = m_height; y < newHeight; ++y) {
        m_grid.push_back(std::vector<TileType>(newWidth, TileType::Empty));
    }

    m_width = newWidth;
    m_height = newHeight;
}

void TileMap::setTile(int gx, int gy, TileType type) {
    if (gx < 0 || gy < 0) return;
    if (gx >= m_width || gy >= m_height) {
        expandToFit(gx + 10, gy + 1);
    }
    m_grid[gy][gx] = type;
}
