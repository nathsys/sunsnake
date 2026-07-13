#include <iostream>
#include <sstream>
#include <vector>
#include "defaults.hpp"
#include "dynamic_tables.hpp"
#include "move_logic.hpp"
#include "testing.hpp"
#include "move_generator.hpp"
#include "move_counter.hpp"
#include "util.hpp"

static void try_make_move(std::string move_str, Board &board, GameState &state)
{
    MoveStack move_stack;
    FlagStack flag_stack;
    MoveList moves = state.player() == Player::white ? move_generation::generate_moves<Player::white>(board, state, move_stack) : move_generation::generate_moves<Player::black>(board, state, move_stack);
    for (Move &move : moves)
    {
        if (MoveEncoding::move_to_string(move) == move_str)
        {
            std::cerr << "Making move: " << move_str << '\n';
            if (state.player() == Player::white)
                make_move::make_move<Player::white>(board, state, move, flag_stack);
            else
                make_move::make_move<Player::black>(board, state, move, flag_stack);
            return;
        }
    }
    std::cerr << "Move not found!\n";
    return;
}

static bool process_command(const std::vector<std::string> &args, Board &board, GameState &gamestate, int &i)
{
    if (args.empty())
        return true;

    std::string command = args[0];

    if (command == "quit" || command == "exit")
    {
        return false;
    }
    else if (command == "perft" && args.size() >= 2)
    {
        int depth = std::stoi(args[1]);
        std::cerr << "Running perft to depth " << depth << " from current position\n";
        move_counting::perft(board, gamestate, depth);
        i += 1; // For command line mode, skip the next argument
    }
    else if (command == "move" && args.size() >= 2)
    {
        std::string move_str = args[1];
        try_make_move(move_str, board, gamestate);
        i += 1; // For command line mode, skip the next argument
    }
    else if (command == "print")
    {
        board.print_board();
    }
    else
    {
        std::cerr << "Unknown command or insufficient arguments. Available commands:\n";
        std::cerr << "  perft <depth>        - Run perft to specified depth from current position\n";
        std::cerr << "  move <move>          - Make a move (e.g., e2e4)\n";
        std::cerr << "  print                - Print current board\n";
        std::cerr << "  state                - Print game state\n";
        std::cerr << "  quit                 - Exit program\n";
    }

    return true;
}

int main(int argc, char **argv)
{
#ifdef run_tests
    Testing::run_all_tests();
#endif

#ifdef generate_magics
    std::cout << "\nGenerating magic numbers for rook.\n"
              << std::endl;
    Lookup::init_magics<SliderType::orthogonal>();
    std::cout << "\nGenerating magic numbers for bishop.\n"
              << std::endl;
    Lookup::init_magics<SliderType::diagonal>();
    return 0;
#endif

    Board board;
    GameState gamestate = get_initial_gamestate();

    if (argc > 1)
    {
        // Command line mode: run the given commands and exit. stdout is left
        // clean (just the data each command prints) so it's easy to pipe
        // into a script; status/progress messages go to stderr instead.
        for (int i = 1; i < argc; ++i)
        {
            std::vector<std::string> args;
            for (int j = i; j < argc; ++j)
            {
                args.push_back(argv[j]);
            }

            int skip_count = 0;
            if (!process_command(args, board, gamestate, skip_count))
            {
                return 0;
            }
            i += skip_count;
        }
        return 0;
    }

    // Interactive mode
    board.print_board();
    std::cout << "\nEntering interactive mode. Available commands:\n";
    std::cout << "  perft <depth>        - Run perft to specified depth from current position\n";
    std::cout << "  move <move>          - Make a move (e.g., e2e4)\n";
    std::cout << "  print                - Print current board\n";
    std::cout << "  state                - Print game state\n";
    std::cout << "  quit                 - Exit program\n";
    std::cout << "> ";

    while (true)
    {
        std::string input;
        if (!std::getline(std::cin, input))
            break;

        if (input.empty())
        {
            std::cout << "> ";
            continue;
        }

        // Parse the input into tokens
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream iss(input);
        while (iss >> token)
        {
            tokens.push_back(token);
        }

        if (tokens.empty())
        {
            std::cout << "> ";
            continue;
        }

        int dummy = 0; // Not used in interactive mode
        if (!process_command(tokens, board, gamestate, dummy))
        {
            break; // Exit if quit/exit command was issued
        }
        std::cout << "> ";
    }
}