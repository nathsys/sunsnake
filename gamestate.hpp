#include "defaults.hpp"
#include <bitset>
#include <cstdint>
#include <iostream>

struct GameState {
    // From least significant to most significant bit:
    // 0 - castling rights for white king side.
    // 1 - castling rights for white queen side.
    // 2 - castling rights for black king side.
    // 3 - castling rights for black queen side.
    // 4 - en passant possible or not.
    // 5 - white to move.
    uint8_t state = 0b0000'1111;

    constexpr GameState(uint8_t state, Player player = Player::white, bool en_passant = false){
        this->state = state;
        if (player == Player::black) {
            this->state |= 0b0010'0000; // Set black to move.
        }
        if (en_passant) {
            this->state |= 0b0001'0000; // Set en passant.
        }
    }

    constexpr Player player() const {
        return (state & 0b0010'0000) ? Player::white : Player::black;
    }

    constexpr GameState switch_player() const {
        return GameState(state ^ 0b0010'0000);
    }

    constexpr GameState can_ep_next_ply() const {
        return GameState(state | 0b0001'0000);
    }

    constexpr bool can_ep() const {
        return state & 0b0001'0000;
    }

    template<Player player>
    constexpr GameState rm_cr() const {
        uint8_t new_state = state;
        if constexpr (player == Player::white) {
            new_state &= 0b1111'1100; // Remove white castling rights.
        } else {
            new_state &= 0b1111'0011; // Remove black castling rights.
        }
        return GameState(new_state);
    }

    void print_state_data() const {
        std::cout << "Castling rights: " << std::bitset<4>(state & 0b0000'1111) << '\n';
        std::cout << "En passant: " << can_ep() << '\n';
        std::cout << "Current player: " << (player() == Player::white ? "White" : "Black") << '\n';
    }

};

constexpr GameState get_initial_gamestate() {
    return GameState(0b0010'1111); // Initial state with all castling rights and white to move.
}
