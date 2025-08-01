#include <iostream>
#include "defaults.hpp"
#include "dynamic_tables.hpp"
#include "move_logic.hpp"
#include "testing.hpp"
#include "move_counter.hpp"
#include "util.hpp"

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
    board.print_board();

    while (true) {
        std::cout << "Enter a command (q to quit): ";
        std::string command;
        std::getline(std::cin, command);
        if (command == "q") break;

        if (command == "print") {
            board.print_board();
        } else if (command == "perft") {
            move_counting::count<get_initial_gamestate()>(board, 7);
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }
}