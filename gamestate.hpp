#include "defaults.hpp"
#include <bitset>
#include <cstdint>
#include <iostream>

struct GameState
{
    bool w_can_castle_k = true;
    bool w_can_castle_q = true;
    bool b_can_castle_k = true;
    bool b_can_castle_q = true;
    Player current_player = Player::white;
    bool en_passant = false;
    PieceType last_captured = PieceType::none;

    constexpr GameState(Player player = Player::white, bool en_passant = false,
                        bool w_can_castle_k = true, bool w_can_castle_q = true,
                        bool b_can_castle_k = true, bool b_can_castle_q = true) : w_can_castle_k(w_can_castle_k), w_can_castle_q(w_can_castle_q),
                                                                                  b_can_castle_k(b_can_castle_k), b_can_castle_q(b_can_castle_q),
                                                                                  current_player(player), en_passant(en_passant)
    {
    }

    constexpr Player player() const
    {
        return current_player;
    }

    constexpr GameState switch_player() const
    {
        return GameState(current_player == Player::white ? Player::black : Player::white, en_passant,
                         w_can_castle_k, w_can_castle_q, b_can_castle_k, b_can_castle_q);
    }

    constexpr GameState can_ep_next_ply() const
    {
        return GameState(current_player, true, w_can_castle_k, w_can_castle_q, b_can_castle_k, b_can_castle_q);
    }

    constexpr bool can_ep() const
    {
        return en_passant;
    }

    template <Player player>
    constexpr GameState rm_cr() const
    {
        bool new_w_can_castle_k = w_can_castle_k;
        bool new_w_can_castle_q = w_can_castle_q;
        bool new_b_can_castle_k = b_can_castle_k;
        bool new_b_can_castle_q = b_can_castle_q;
        if constexpr (player == Player::white)
        {
            new_w_can_castle_k = false;
            new_w_can_castle_q = false;
        }
        else
        {
            new_b_can_castle_k = false;
            new_b_can_castle_q = false;
        }

        return GameState(current_player, en_passant, new_w_can_castle_k, new_w_can_castle_q, new_b_can_castle_k, new_b_can_castle_q);
    }

    template <Player player, bool kingside>
    constexpr GameState rm_cr() const
    {
        bool new_w_can_castle_k = w_can_castle_k;
        bool new_w_can_castle_q = w_can_castle_q;
        bool new_b_can_castle_k = b_can_castle_k;
        bool new_b_can_castle_q = b_can_castle_q;
        if constexpr (player == Player::white)
        {
            if constexpr (kingside)
                new_w_can_castle_k = false;
            else
                new_w_can_castle_q = false;
        }
        else
        {
            if constexpr (kingside)
                new_b_can_castle_k = false;
            else
                new_b_can_castle_q = false;
        }

        return GameState(current_player, en_passant, new_w_can_castle_k, new_w_can_castle_q, new_b_can_castle_k, new_b_can_castle_q);
    }

    void print_state_data() const
    {
        std::cout << "Current player: " << (current_player == Player::white ? "White" : "Black") << '\n';
        std::cout << "Can white castle kingside: " << w_can_castle_k << '\n';
        std::cout << "Can white castle queenside: " << w_can_castle_q << '\n';
        std::cout << "Can black castle kingside: " << b_can_castle_k << '\n';
        std::cout << "Can black castle queenside: " << b_can_castle_q << '\n';
        std::cout << "En passant available: " << en_passant << '\n';
    }
};

constexpr GameState get_initial_gamestate()
{
    return GameState(Player::white, false, true, true, true, true);
}

GameState parse_fen_to_gamestate(std::string fen)
{
    GameState state = get_initial_gamestate();
    size_t space1 = fen.find(' ');
    size_t space2 = fen.find(' ', space1 + 1);
    size_t space3 = fen.find(' ', space2 + 1);
    size_t space4 = fen.find(' ', space3 + 1);
    size_t space5 = fen.find(' ', space4 + 1);

    std::string board_part = fen.substr(0, space1);
    std::string player_part = fen.substr(space1 + 1, space2 - space1 - 1);
    std::string castling_part = fen.substr(space2 + 1, space3 - space2 - 1);
    std::string en_passant_part = fen.substr(space3 + 1, space4 - space3 - 1);

    // Current player
    if (player_part == "w")
    {
        state.current_player = Player::white;
    }
    else if (player_part == "b")
    {
        state.current_player = Player::black;
    }
    else
    {
        throw std::runtime_error("Invalid FEN: Unknown player.");
    }

    // Castling rights
    state.w_can_castle_k = castling_part.find('K') != std::string::npos;
    state.w_can_castle_q = castling_part.find('Q') != std::string::npos;
    state.b_can_castle_k = castling_part.find('k') != std::string::npos;
    state.b_can_castle_q = castling_part.find('q') != std::string::npos;

    // En passant
    state.en_passant = en_passant_part != "-";

    return state;
}