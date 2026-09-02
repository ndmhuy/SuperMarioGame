#pragma once

class Entity;

// The door an editor command must use to add an entity to, or take one out of,
// a live game state.
//
// Why this exists
// ---------------
// PlaceEntityCommand used to push straight into the entity vector and
// EraseEntityCommand used to destroy whatever it was handed. Both bypassed the
// owning state entirely, and the owning state is the only thing that knows two
// facts the command cannot:
//
//   1. An entity that has not had setupAnimations() called on it has no sprite,
//      so Entity::render falls through to drawPlaceholder() — a flat coloured
//      box. Every entity the editor placed drew as one (R21 lane E).
//   2. PlayingState::m_player, Game::setPlayer and InputManager::registerPlayer
//      hold RAW pointers into that vector. Destroying a Player from the editor
//      left all three dangling (audit A-3, and the editor was still doing it).
//
// Implementations are expected to be the state that owns the vector, so the
// contract is deliberately narrow: admit() must leave the entity fully drawable,
// release() must have dropped every observer pointer into it by the time it
// returns. Neither takes ownership.
class IEntityAdmitter {
public:
    virtual ~IEntityAdmitter() = default;

    // Called with an entity that has just entered the world and is already in
    // the entity vector.
    virtual void admit(Entity* entity) = 0;

    // Called with an entity that is about to leave the vector. It is still alive
    // for the duration of the call.
    virtual void release(Entity* entity) = 0;
};
