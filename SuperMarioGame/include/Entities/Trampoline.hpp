#pragma once

#include "Entities/Item.hpp"

class Trampoline : public Item {
public:
    explicit Trampoline(sf::Vector2f pos);
    ~Trampoline() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void collect() override;

    bool isCompressed() const { return m_isCompressed; }

private:
    float m_compressTimer = 0.0f;
    bool m_isCompressed = false;
};
