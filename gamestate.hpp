#include "defaults.hpp"
#include <cstdint>

struct GameState {
    constexpr GameState(uint8_t state)
        : state(state) {}

    constexpr Player get_player() const {
        return (state & 0b0010'0000) ? Player::white : Player::black;
    }

    constexpr GameState switch_player() const {
        return GameState(state ^ 0b0010'0000);
    }

    // From least significant to most significant bit:
    // 0 - castling rights for white king side.
    // 1 - castling rights for white queen side.
    // 2 - castling rights for black king side.
    // 3 - castling rights for black queen side.
    // 4 - en passant possible or not.
    // 5 - white to move.
    uint8_t state = 0b0000'1111;
};
