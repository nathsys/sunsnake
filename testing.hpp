#include <cassert>

#include "defaults.hpp"
#include "util.hpp"
#include "dynamic_tables.hpp"
#include "move_logic.hpp"

#pragma once

#ifdef run_tests
namespace Testing {
    // Test if attack on fly produces the same results as the magic number 
    // based attack generation.
    static void test_magic_atk_lookup() {
        for(int i = 0; i < 64; i++) {
            for(BitBoard j = 0; j < 64; j++) {
                BitBoard j_mask = square_to_mask(j);
                // Test bishop attack logic.
                BitBoard b1 = Lookup::bishop_attack_on_fly(i, j_mask);
                BitBoard b2 = get_bishop_move(i, j_mask);
                if (b1 != b2) {
                    std::cout << "Bishop failed at sq=" << i << " occ=" << j << std::endl;
                    std::cout << "On-the-fly:" << std::endl;
                    print_bitboard(b1);
                    std::cout << "Magic:" << std::endl;
                    print_bitboard(b2);
                    std::cout << "Blocker:" << std::endl;
                    print_bitboard(j_mask);
                    assert(b1 == b2 && "Bishop attack logic failed!");
                }

                // Test rook attack logic.
                BitBoard r1 = Lookup::rook_attack_on_fly(i, j_mask);
                BitBoard r2 = get_rook_move(i, j_mask);
                if (r1 != r2) {
                    std::cout << "Rook failed at sq=" << i << " occ=" << j << std::endl;
                    std::cout << "On-the-fly:" << std::endl;
                    print_bitboard(r1);
                    std::cout << "Magic:" << std::endl;
                    print_bitboard(r2);
                    std::cout << "Blocker:" << std::endl;
                    print_bitboard(j_mask);
                    assert(r1 == r2 && "Rook attack logic failed!");
                }
            }
        }
    }

    static void run_all_tests() {
        test_magic_atk_lookup();
        std::cout << "All tests passed!" << std::endl;
    }
}
#endif