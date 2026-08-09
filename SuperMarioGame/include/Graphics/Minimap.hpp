#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <SFML/Graphics.hpp>
#include "Core/EventBus.hpp"

class Entity;
class TileMap;
class Player;

class Minimap final : public sf::Drawable {
public:
    Minimap(sf::Vector2f position, sf::Vector2f size);
    ~Minimap();

    void update(float dt, const Player* player, const std::vector<std::unique_ptr<Entity>>& entityList);

    void initialize(const TileMap& tileMap);

private:
    enum class EntityType {
        Player,
        Enemy,
        Item
    };

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::Vector2f m_pos, m_size;
    sf::Texture m_mapTexture;
    bool m_visible;
    EventBus::SubscriptionId m_toggleId;
    std::vector<std::pair<sf::Vector2f,EntityType>> m_entityList;
};