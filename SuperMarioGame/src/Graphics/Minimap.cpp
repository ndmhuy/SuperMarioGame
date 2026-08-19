#include "Graphics/Minimap.hpp"

#include <iostream>
#include <SFML/Graphics/Color.hpp>
#include "Graphics/ColorPalette.hpp"
#include "Entities/Player.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"

Minimap::Minimap(sf::Vector2f position, sf::Vector2f size) 
    : m_pos(position), m_size(size), m_visible(false) {
        m_toggleId = EventBus::getInstance().subscribe(
            EventType::MinimapToggled, 
            [this](const GameEvent& event) {
                m_visible = !m_visible;
            }
        );   
}

Minimap::~Minimap() {
    EventBus::getInstance().unsubscribe(m_toggleId);
}

void Minimap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!m_visible) return;
    sf::RectangleShape backgroundFrame(m_size);
    backgroundFrame.setPosition(m_pos);
    backgroundFrame.setFillColor(sf::Color(0,0,0,100));

    float scaleX = m_size.x / m_mapTexture.getSize().x;
    float scaleY = m_size.y / m_mapTexture.getSize().y;

    sf::Sprite mapSprite(m_mapTexture);
    mapSprite.setPosition(m_pos);
    mapSprite.setScale({scaleX, scaleY});

    target.draw(backgroundFrame,states);
    target.draw(mapSprite, states);

    for (const std::pair<sf::Vector2f,EntityType> &entity : m_entityList) {
        sf::CircleShape entityDot(Constants::ENTITY_DOT_RADIUS);
        entityDot.setOrigin({Constants::ENTITY_DOT_RADIUS, Constants::ENTITY_DOT_RADIUS});
        
        float tileX = entity.first.x / Constants::TILE_SIZE;
        float tileY = entity.first.y / Constants::TILE_SIZE;
        
        entityDot.setPosition({
            m_pos.x + tileX * scaleX,
            m_pos.y + tileY * scaleY
        });

        switch (entity.second)
        {
        case EntityType::Player:
            // Was Constants::MINIMAP_PLAYER_DOT_COLOR (green) against a red enemy
            // dot — the classic red/green pair (task 11.4).
            entityDot.setFillColor(ColorPalette::get(ColorPalette::Role::Player));
            break;
            
        case EntityType::Enemy:
            entityDot.setFillColor(ColorPalette::get(ColorPalette::Role::Enemy));
            break;
            
        case EntityType::Item:
            entityDot.setFillColor(ColorPalette::get(ColorPalette::Role::Item));
            break;
        }

        target.draw(entityDot);
    }

}

void Minimap::update(float dt, const Player* player, const std::vector<std::unique_ptr<Entity>>& entityList) {
    m_entityList.clear();
    if (player) {
        m_entityList.emplace_back(player->getPosition(), EntityType::Player);
    }
    for (const std::unique_ptr<Entity> &entity : entityList) {
        if (!entity->isActive()) continue;
        if (dynamic_cast<const Enemy*>(entity.get())) {
            m_entityList.emplace_back(entity->getPosition(), EntityType::Enemy);
        }
        else if (dynamic_cast<const Item*>(entity.get())) {
            m_entityList.emplace_back(entity->getPosition(), EntityType::Item);
        }
    }
}

void Minimap::initialize(const TileMap& tileMap) {
    sf::Image mapImage(
        sf::Vector2u(tileMap.getWidth(), tileMap.getHeight()),
        sf::Color::Transparent
    );

    for (int i=0;i<tileMap.getWidth();i++) {
        for (int j=0;j<tileMap.getHeight();j++) {
            TileType tileType = tileMap.getTileType(i,j);
            if (tileMap.getInfo(tileType).isSolid) {
                mapImage.setPixel(sf::Vector2u(i,j), Constants::MINIMAP_SOLID_BLOCK_COLOR);
            }
        }
    }

    if(!m_mapTexture.loadFromImage(mapImage)) {
        
    }
}
