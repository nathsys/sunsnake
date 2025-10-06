#include "defaults.hpp"
#include <bitset>
#include <cstdint>
#include <iostream>

struct GameState
{
    // 4 bits for en passant file (0 if none, 1-8 for a-h)
    // 1 bit each for white kingside, white queenside, black kingside, black queenside castling rights.
    uint data = 00001111;
    Player current_player = Player::white;

    constexpr GameState(Player player = Player::white, u_int8_t en_passant = 0,
                        bool w_can_castle_k = true, bool w_can_castle_q = true,
                        bool b_can_castle_k = true, bool b_can_castle_q = true) : data(0b00001111), current_player(player)
    {
        if (en_passant > 0)
            data |= (en_passant & 0b11110000); // Set the first 4 bits to the en passant file.

        if (!w_can_castle_k)
            data &= 0b11111110; // Clear bit 4.
        if (!w_can_castle_q)
            data &= 0b11111101; // Clear bit 5.
        if (!b_can_castle_k)
            data &= 0b11111011; // Clear bit 6.
        if (!b_can_castle_q)
            data &= 0b11110111; // Clear bit 7.
    }

    constexpr Player player() const
    {
        return current_player;
    }

    constexpr void switch_player()
    {
        current_player = (current_player == Player::white) ? Player::black : Player::white;
    }

    constexpr bool can_ep() const
    {
        return data & 0b11110000;
    }

    template <Player player>
    constexpr void rm_cr()
    {
        if constexpr (player == Player::white)
        {
            data &= 0b11111100; // Clear bits 4 and 5.
        }
        else
        {
            data &= 0b11110011; // Clear bits 6 and 7.
        }
    }

    template <Player player, bool kingside>
    constexpr void rm_cr()
    {
        if constexpr (player == Player::white)
        {
            if constexpr (kingside)
                data &= 0b11111110; // Clear bit 4.
            else
                data &= 0b11111101; // Clear bit 5.
        }
        else
        {
            if constexpr (kingside)
                data &= 0b11111011; // Clear bit 6.
            else
                data &= 0b11110111; // Clear bit 7.
        }
    }
};

constexpr GameState get_initial_gamestate()
{
    return GameState(Player::white, 0, true, true, true, true);
}