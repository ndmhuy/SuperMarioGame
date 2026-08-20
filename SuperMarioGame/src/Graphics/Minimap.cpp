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

        case EntityType::Rival:
            // The item colour rather than the enemy colour: the other player is
            // a competitor, not a hazard, and the enemy red is already spoken
            // for by everything that can actually kill you.
            entityDot.setFillColor(ColorPalette::get(ColorPalette::Role::Item));
            break;

        case EntityType::Shadow:
            // Not from the palette: the shadow's purple is the one colour in the
            // game that means "this is you, and it will hurt you", and the
            // colourblind palette has no role for that.
            entityDot.setFillColor(sf::Color(180, 110, 255));
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
        // Other players. The loop used to cast for Enemy and Item only, so in a
        // two-player match the minimap showed one player and every level hazard
        // — the one thing a player checks a minimap for in versus is where the
        // other one is. Detected here rather than passed in, so a rival, a CPU
        // opponent and a shadow all appear without a new parameter each.
        else if (auto* other = dynamic_cast<const Player*>(entity.get())) {
            if (other == player) continue;   // already placed above
            m_entityList.emplace_back(other->getPosition(),
                                      other->isContactHazard() ? EntityType::Shadow
                                                               : EntityType::Rival);
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
