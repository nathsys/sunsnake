#include "defaults.hpp"
#include "gamestate.hpp"
#include "make_move.hpp"
#include "move_generator.hpp"
#include "util.hpp"
#include "movelist.hpp"
#include "move.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>
#include <locale>
#include <iomanip>

#pragma once

#ifdef move_gen

namespace sunsnake
{

namespace move_counting
{
    template <Player player, bool is_root = false>
    static size_t perft(Board &board, GameState &state, int depth, MoveStack &move_stack, FlagStack &data_stack)
    {
        size_t nodes = 0;
        if (depth == 1 && !is_root)
            return move_generation::generate_moves<player>(board, state, move_stack).size();
        for (Move &move : move_generation::generate_moves<player>(board, state, move_stack))
        {
            make_move::make_move<player>(board, state, move, data_stack);
            size_t local = depth - 1 ? perft<opponent<player>()>(board, state, depth - 1, move_stack, data_stack) : 1;
            make_move::unmake_move<player>(board, state, move, data_stack);

            if constexpr (is_root)
                print_movecount(move, local);

            nodes += local;
        }

        return nodes;
    }

    // Parses the depth to a template parameter and calls the templated count function.
    static void perft(Board &board, GameState state, int depth)
    {
        size_t nodes;
        MoveStack move_stack;
        FlagStack data_stack;

        auto start_time = std::chrono::high_resolution_clock::now();

        if (state.player() == Player::white)
            nodes = perft<Player::white, true>(board, state, depth, move_stack, data_stack);
        else
            nodes = perft<Player::black, true>(board, state, depth, move_stack, data_stack);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        double seconds = duration.count() / 1000000.0;
        double nps = seconds > 0 ? nodes / seconds : 0;

        std::cout << "\nNodes searched: " << format_number(nodes) << '\n';

        std::cout << "Time: " << std::fixed << std::setprecision(3) << seconds << " seconds\n";
        std::cout << "Nodes per second: " << format_number(static_cast<size_t>(nps)) << " NPS\n";
    }

} // namespace move_counting

} // namespace sunsnake

#endif // move_gen
