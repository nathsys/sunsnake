// Minimal worked example of using sunsnake as a move generation backend.
// Shows the generate -> make -> recurse -> unmake pattern a search needs,
// plus root move ordering by a naive material-only negamax with a
// transposition table keyed by GameState::hash.
#include <chrono>
#include <iostream>
#include <unordered_map>

#include "sunsnake/chess_board.hpp"
#include "sunsnake/defaults.hpp"
#include "sunsnake/gamestate.hpp"
#include "sunsnake/make_move.hpp"
#include "sunsnake/move.hpp"
#include "sunsnake/move_generator.hpp"
#include "sunsnake/movelist.hpp"
#include "sunsnake/util.hpp"
#include "sunsnake/zobrist.hpp"

using namespace sunsnake;

constexpr int c_mate_score = 30000;
constexpr int c_piece_value[] = {100, 320, 330, 500, 900, 0}; // pawn..king

// depth is how many plies below this entry the score was searched to, so a
// shallower cached entry can't be reused to answer a deeper query.
struct TTEntry
{
    int depth;
    int score;
};

template <Player player>
int material_balance(const Board &board)
{
    int own = 0, opp = 0;
    own += c_piece_value[0] * pcnt(board.get_piece_board<player, PieceType::pawn>());
    own += c_piece_value[1] * pcnt(board.get_piece_board<player, PieceType::knight>());
    own += c_piece_value[2] * pcnt(board.get_piece_board<player, PieceType::bishop>());
    own += c_piece_value[3] * pcnt(board.get_piece_board<player, PieceType::rook>());
    own += c_piece_value[4] * pcnt(board.get_piece_board<player, PieceType::queen>());
    opp += c_piece_value[0] * pcnt(board.get_piece_board<opponent<player>(), PieceType::pawn>());
    opp += c_piece_value[1] * pcnt(board.get_piece_board<opponent<player>(), PieceType::knight>());
    opp += c_piece_value[2] * pcnt(board.get_piece_board<opponent<player>(), PieceType::bishop>());
    opp += c_piece_value[3] * pcnt(board.get_piece_board<opponent<player>(), PieceType::rook>());
    opp += c_piece_value[4] * pcnt(board.get_piece_board<opponent<player>(), PieceType::queen>());
    return own - opp;
}

template <Player player>
int negamax(Board &board, GameState &state, int depth, MoveStack &move_stack, FlagStack &flag_stack, std::unordered_map<Hash, TTEntry> &tt)
{
    auto cached = tt.find(state.hash);
    if (cached != tt.end() && cached->second.depth >= depth)
        return cached->second.score;

    MoveList moves = move_generation::generate_moves<player>(board, state, move_stack);

    if (moves.size() == 0)
    {
        const Square king_sq = lbr(board.get_piece_board<player, PieceType::king>());
        return board.is_attacked<player>(king_sq) ? -c_mate_score - depth : 0;
    }

    if (depth == 0)
        return material_balance<player>(board);

    int best = -c_mate_score - 1;
    for (Move &move : moves)
    {
        make_move::make_move<player>(board, state, move, flag_stack);
        int score = -negamax<opponent<player>()>(board, state, depth - 1, move_stack, flag_stack, tt);
        make_move::unmake_move<player>(board, state, move, flag_stack);
        if (score > best)
            best = score;
    }

    tt[state.hash] = {depth, best};
    return best;
}

// The recursive core stays templated on Player for speed (same approach as
// the library's own move_counting::perft). The root call is the one place a
// caller typically only has a runtime GameState, so it uses the non-template
// generate_moves/make_move/unmake_move overloads instead.
Move search_root(Board &board, GameState &state, int depth, MoveStack &move_stack, FlagStack &flag_stack, std::unordered_map<Hash, TTEntry> &tt)
{
    MoveList moves = move_generation::generate_moves(board, state, move_stack);
    Move best_move = 0;
    int best_score = -c_mate_score - 1;

    for (Move &move : moves)
    {
        make_move::make_move(board, state, move, flag_stack);
        int score = state.player() == Player::white
                        ? -negamax<Player::white>(board, state, depth - 1, move_stack, flag_stack, tt)
                        : -negamax<Player::black>(board, state, depth - 1, move_stack, flag_stack, tt);
        make_move::unmake_move(board, state, move, flag_stack);

        if (score > best_score)
        {
            best_score = score;
            best_move = move;
        }
    }

    std::cout << "bestmove " << MoveEncoding::move_to_string(best_move) << " score " << best_score << '\n';
    return best_move;
}

int main(int argc, char **argv)
{
    const int depth = argc > 1 ? std::stoi(argv[1]) : 4;

    Board board;
    GameState state = get_initial_gamestate();
    MoveStack move_stack;
    FlagStack flag_stack;
    std::unordered_map<Hash, TTEntry> tt;

    auto start = std::chrono::high_resolution_clock::now();

    search_root(board, state, depth, move_stack, flag_stack, tt);

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "depth " << depth << " in " << ms << " ms, " << tt.size() << " transposition table entries\n";

    return 0;
}
