#include <cassert>

#include "defaults.hpp"
#include "util.hpp"
#include "dynamic_tables.hpp"
#include "move_logic.hpp"
#include "gamestate.hpp"
#include "make_move.hpp"
#include "move_generator.hpp"
#include "zobrist.hpp"

#pragma once

#ifdef run_tests

namespace sunsnake
{

// *= A suite of testing functions used to verify program integrity *=

namespace Testing {
    // Test if attack on fly produces the same results as the magic number 
    // based attack generation.
    static void test_magic_atk_lookup() {
        for(int i = 0; i < 64; i++) {
            for(BitBoard j = 0; j < 1000; j++) {
                // Test bishop attack logic.
                BitBoard b1 = Lookup::bishop_attack_on_fly(i, j);
                BitBoard b2 = get_bishop_move(i, j);
                if (b1 != b2) {
                    std::cout << "Bishop failed at sq=" << i << " occ=" << j << std::endl;
                    std::cout << "On-the-fly:" << std::endl;
                    print_bitboard(b1);
                    std::cout << "Magic:" << std::endl;
                    print_bitboard(b2);
                    std::cout << "Blocker:" << std::endl;
                    print_bitboard(j);
                    assert(b1 == b2 && "Bishop attack logic failed!");
                }

                // Test rook attack logic.
                BitBoard r1 = Lookup::rook_attack_on_fly(i, j);
                BitBoard r2 = get_rook_move(i, j);
                if (r1 != r2) {
                    std::cout << "Rook failed at sq=" << i << " occ=" << j << std::endl;
                    std::cout << "On-the-fly:" << std::endl;
                    print_bitboard(r1);
                    std::cout << "Magic:" << std::endl;
                    print_bitboard(r2);
                    std::cout << "Blocker:" << std::endl;
                    print_bitboard(j);
                    assert(r1 == r2 && "Rook attack logic failed!");
                }
            }
        }
    }

    // Walks the move tree, checking at every node that the incrementally
    // maintained Zobrist hash matches an independent from-scratch recompute,
    // and that unmaking a move restores the exact pre-move hash.
    template <Player player>
    static void walk_zobrist(Board &board, GameState &state, int depth, MoveStack &move_stack, FlagStack &flag_stack) {
        const Hash expected = Zobrist::compute(board, state.data, state.player());
        assert(state.hash == expected && "Zobrist hash does not match a from-scratch recompute!");

        if (depth == 0)
            return;

        for (Move &move : move_generation::generate_moves<player>(board, state, move_stack)) {
            const Hash before = state.hash;
            make_move::make_move<player>(board, state, move, flag_stack);
            walk_zobrist<opponent<player>()>(board, state, depth - 1, move_stack, flag_stack);
            make_move::unmake_move<player>(board, state, move, flag_stack);
            assert(state.hash == before && "Zobrist hash was not restored after unmake_move!");
        }
    }

    static void test_zobrist_consistency() {
        Board board;
        GameState state = get_initial_gamestate();
        MoveStack move_stack;
        FlagStack flag_stack;
        walk_zobrist<Player::white>(board, state, 4, move_stack, flag_stack);
    }

    static void run_all_tests() {
        test_magic_atk_lookup();
        test_zobrist_consistency();
        std::cout << "All tests passed!" << std::endl;
    }
}

} // namespace sunsnake

#endif