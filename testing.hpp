
#include <cassert>

#include "defaults.hpp"
#include "util.hpp"
#include "dynamic_tables.hpp"
#include "move_logic.hpp"

#pragma once

namespace Testing {
    static void test_slider_logic() {
        print_bitboard(Lookup::rook_attacks[0][0]);
        assert(get_bishop_move(0, 0) == Lookup::bishop_attack_on_fly(0, 0));
        assert(get_bishop_move(0, INIT_PAWN_SQUARES) == get_bishop_move(0, INIT_PAWN_SQUARES));
        
        print_bitboard(get_rook_move(0, 0));
        print_bitboard(get_rook_move(0, INIT_PAWN_SQUARES));
        print_bitboard(Lookup::rook_attack_on_fly(0, INIT_PAWN_SQUARES));
        assert(get_rook_move(0, 0) == Lookup::rook_attack_on_fly(0, 0));
        assert(get_rook_move(0, INIT_PAWN_SQUARES) == Lookup::rook_attack_on_fly(0, INIT_PAWN_SQUARES));
    }

    static void run_all_tests() {
        test_slider_logic();
    }
 }