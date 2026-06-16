#pragma once

#include <vector>
#include <SFML/Graphics/RenderTarget.hpp>
#include "Physics/AABB.hpp"

class Camera;

class TileMap {
public:
    TileMap() = default;
    ~TileMap() = default;

    // Loader & Renderers
    void render(sf::RenderTarget& target, Camera& camera);

    // Grid Helpers
    int getTileAt(float px, float py) const;
    sf::Vector2i worldToGrid(float px, float py) const;
    sf::Vector2f gridToWorld(int gx, int gy) const;

    // Surface modifiers
    int getTileSurfaceType(float px, float py) const;
    void swapBricksAndCoins();

    // Initialization for testing
    void initialize(int width, int height);
    void setTile(int gx, int gy, int type);
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getTileType(int gx, int gy) const {
        if (gx >= 0 && gx < m_width && gy >= 0 && gy < m_height) {
            return m_grid[gy][gx];
        }
        return 0;
    }

private:
    int m_width = 0;
    int m_height = 0;
    std::vector<std::vector<int>> m_grid;
};
