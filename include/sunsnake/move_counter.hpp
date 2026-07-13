#include "defaults.hpp"
#include "gamestate.hpp"
#include "make_move.hpp"
#include "move_generator.hpp"
#include "util.hpp"
#include "movelist.hpp"
#include "move.hpp"
#include "zobrist.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>
#include <locale>
#include <iomanip>
#include <unordered_map>

#pragma once

#ifdef move_gen

namespace sunsnake
{

namespace move_counting
{
    // use_hash caches subtree node counts by (Zobrist hash, depth remaining).
    // It exists to validate the hash, not to go faster: if two different
    // positions ever collided, or the incremental update were wrong, a
    // transposition would reuse the wrong cached count and the total would
    // diverge from a plain (use_hash = false) run.
    template <Player player, bool is_root = false, bool use_hash = false>
    static size_t perft(Board &board, GameState &state, int depth, MoveStack &move_stack, FlagStack &data_stack, std::unordered_map<Hash, size_t> *table = nullptr)
    {
        Hash key = 0;
        if constexpr (use_hash && !is_root)
        {
            key = state.hash ^ Zobrist::depth_key(depth);
            auto it = table->find(key);
            if (it != table->end())
                return it->second;
        }

        size_t nodes;
        if (depth == 1 && !is_root)
        {
            nodes = move_generation::generate_moves<player>(board, state, move_stack).size();
        }
        else
        {
            nodes = 0;
            for (Move &move : move_generation::generate_moves<player>(board, state, move_stack))
            {
                make_move::make_move<player>(board, state, move, data_stack);
                size_t local = depth - 1 ? perft<opponent<player>(), false, use_hash>(board, state, depth - 1, move_stack, data_stack, table) : 1;
                make_move::unmake_move<player>(board, state, move, data_stack);

                if constexpr (is_root)
                    print_movecount(move, local);

                nodes += local;
            }
        }

        if constexpr (use_hash && !is_root)
            (*table)[key] = nodes;

        return nodes;
    }

    // Parses the depth to a template parameter and calls the templated count
    // function. use_hash runs the exact same search through the Zobrist-hash
    // cached path instead, but produces identical output either way.
    static void perft(Board &board, GameState state, int depth, bool use_hash = false)
    {
        size_t nodes;
        MoveStack move_stack;
        FlagStack data_stack;
        std::unordered_map<Hash, size_t> table;

        auto start_time = std::chrono::high_resolution_clock::now();

        if (use_hash)
        {
            nodes = state.player() == Player::white
                        ? perft<Player::white, true, true>(board, state, depth, move_stack, data_stack, &table)
                        : perft<Player::black, true, true>(board, state, depth, move_stack, data_stack, &table);
        }
        else
        {
            nodes = state.player() == Player::white
                        ? perft<Player::white, true>(board, state, depth, move_stack, data_stack)
                        : perft<Player::black, true>(board, state, depth, move_stack, data_stack);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        double seconds = duration.count() / 1000000.0;
        double nps = seconds > 0 ? nodes / seconds : 0;

        std::cout << "\nNodes searched: " << format_number(nodes) << '\n';

        std::cout << "Time: " << std::fixed << std::setprecision(3) << seconds << " seconds\n";
        std::cout << "Nodes per second: " << format_number(static_cast<size_t>(nps)) << " NPS\n";
    }

    // Runs plain and Zobrist-hashed perft over the same position, timed
    // separately, and reports whether their totals match plus the NPS
    // difference caching transpositions makes.
    static void perft_validate_hash(Board &board, GameState state, int depth)
    {
        MoveStack move_stack;
        FlagStack data_stack;
        std::unordered_map<Hash, size_t> table;

        auto plain_start = std::chrono::high_resolution_clock::now();
        const size_t plain = state.player() == Player::white
                                  ? perft<Player::white>(board, state, depth, move_stack, data_stack)
                                  : perft<Player::black>(board, state, depth, move_stack, data_stack);
        auto plain_end = std::chrono::high_resolution_clock::now();

        auto hashed_start = std::chrono::high_resolution_clock::now();
        const size_t hashed = state.player() == Player::white
                                   ? perft<Player::white, false, true>(board, state, depth, move_stack, data_stack, &table)
                                   : perft<Player::black, false, true>(board, state, depth, move_stack, data_stack, &table);
        auto hashed_end = std::chrono::high_resolution_clock::now();

        const double plain_seconds = std::chrono::duration_cast<std::chrono::microseconds>(plain_end - plain_start).count() / 1000000.0;
        const double hashed_seconds = std::chrono::duration_cast<std::chrono::microseconds>(hashed_end - hashed_start).count() / 1000000.0;
        const double plain_nps = plain_seconds > 0 ? plain / plain_seconds : 0;
        const double hashed_nps = hashed_seconds > 0 ? hashed / hashed_seconds : 0;
        const double nps_diff_percent = plain_nps > 0 ? (hashed_nps - plain_nps) / plain_nps * 100.0 : 0;

        std::cout << "\nPlain nodes:  " << format_number(plain) << '\n';
        std::cout << "Plain NPS:    " << format_number(static_cast<size_t>(plain_nps)) << '\n';
        std::cout << "Hashed nodes: " << format_number(hashed) << '\n';
        std::cout << "Hashed NPS:   " << format_number(static_cast<size_t>(hashed_nps)) << '\n';
        std::cout << "NPS diff:     " << std::fixed << std::setprecision(1) << (nps_diff_percent >= 0 ? "+" : "") << nps_diff_percent << "%\n";
        std::cout << (plain == hashed ? "Zobrist hash validation: match\n" : "Zobrist hash validation: MISMATCH\n");
    }

} // namespace move_counting

} // namespace sunsnake

#endif // move_gen
