#include <iostream>
#include "defaults.hpp"
#include "dynamic_tables.hpp"
#include "testing.hpp"
#include "move_counter.hpp"

int main() {
    #ifdef run_tests 
    Testing::run_all_tests();
    #else
    #ifdef generate_magics
    std::cout << "\nGenerating magic numbers for rook.\n" << std::endl;
    Lookup::init_magics<false>();
    std::cout << "\nGenerating magic numbers for bishop.\n" << std::endl;
    Lookup::init_magics<true>();
    #endif
    #endif

    Board board;
    move_counting::count<3, get_initial_gamestate()>(board);
}