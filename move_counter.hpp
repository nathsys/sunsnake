#include "defaults.hpp"
#include "gamestate.hpp"
#include "make_move.hpp"
#include "maskset.hpp"
#include "util.hpp"
#include <cstdint>

#ifdef count_moves

namespace move_counting {

template<int depth_left, GameState state>
inline uint64_t count_king_moves(const auto& masks, const BitBoard legal_squares, Board& board, const BitBoard king_square) {
    // Get the king move board. Seperate captures and normal moves.
    BitBoard king_moves = get_king_move(king_square) & legal_squares;
    BitBoard king_captures = king_moves & board.get_player_occ<opponent<state.get_player()>>();
    king_moves ^= king_captures;

    // Process the king moves.
    uint64_t ret = 0;
    while(king_moves) {
        BitBoard move = pop_extract(king_moves);
        if(board.is_attacked<state.get_player()>(move)) continue;
        // Make the move, count moves from there, undo move.
        move |= board.get_piece_board<state.get_player(), PieceType::king>();
        move_piece<state.get_player(), PieceType::king>(board, move);
        ret += count_nodes<depth_left - 1, state.switch_player()>(masks, board);
        move_piece<state.get_player(), PieceType::king>(board, move);
    }

    while(king_captures) {
        BitBoard move = pop_extract(king_captures);
        if(board.is_attacked<state.get_player()>(move)) continue;
        PieceType captured = board.get_piece_type<state.get_player()>(move);
        // Make the move, count moves from there, undo move.
        capture_move<state.get_player(), PieceType::king>(board, board.get_piece_board<state.get_player(), PieceType::king>(), move, captured);
    }
}

template <int depth_left, GameState state>
uint64_t count_nodes(const auto &masks, Board &board) {
    const Square king_square = lbit(board.get_piece_board<state.get_player(), PieceType::king>());
    // Make masks.
    MaskSet& mask = masks[depth_left];
    create_masks<state.get_player()>(board, mask, king_square);
    // King moves can always be generated.
    uint64_t ret = generate_king_moves<depth_left, state>(masks, mask.legal_squares, board, king_square);
}

template <int depth, GameState state>
static uint64_t count(Board& board) {
    MaskSet masks[depth + 1];
    // Count moves and time it.
	const clock_t start = clock();
	const uint64_t nodes = count_nodes<depth, state>(masks, board);
	const clock_t end = clock();
    // Print the results.
    std::cout << "================================================================================ \n";
	const double time_cost = double(end - start) / CLOCKS_PER_SEC;
	std::cout << "PERFT results: \nNodes evaluated: " << nodes << "\nTime cost: " << time_cost << '\n';
	std::cout << std::round((nodes / 1'000'000)) / time_cost << " million nodes per second" << '\n';
	std::cout << "================================================================================ \n";
	return nodes;
}

};

#endif // count_moves