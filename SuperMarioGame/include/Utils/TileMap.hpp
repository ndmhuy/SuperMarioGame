#pragma once

#include <vector>
#include <string>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Color.hpp>
#include "Physics/AABB.hpp"

class Camera;

enum class TileType : int {
    Empty = 0,
    Ground = 1,
    Brick = 2,
    Question = 3,
    Pipe = 4,
    Ice = 5,
    Conveyor = 6,
    Water = 7,
    Coin = 8,
    Used = 9
};

struct TileInfo {
    TileType type = TileType::Empty;
    bool isSolid = false;
    sf::Color debugColor = sf::Color::Transparent;
    std::string name = "Empty";
};

class TileMap {
public:
    TileMap() = default;
    ~TileMap() = default;

    // Get centralized tile metadata
    static const TileInfo& getInfo(TileType type);

    // Loader & Renderers
    void render(sf::RenderTarget& target, Camera& camera);

    // Grid Helpers
    TileType getTileAt(float px, float py) const;
    sf::Vector2i worldToGrid(float px, float py) const;
    sf::Vector2f gridToWorld(int gx, int gy) const;

    // Surface modifiers
    TileType getTileSurfaceType(float px, float py) const;
    void swapBricksAndCoins();

    // Initialization for testing
    void initialize(int width, int height);
    void setTile(int gx, int gy, TileType type);
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    TileType getTileType(int gx, int gy) const {
        if (gx >= 0 && gx < m_width && gy >= 0 && gy < m_height) {
            return m_grid[gy][gx];
        }
        return TileType::Empty;
    }

private:
    int m_width = 0;
    int m_height = 0;
    std::vector<std::vector<TileType>> m_grid;
};
