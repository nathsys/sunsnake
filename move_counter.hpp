#include "defaults.hpp"
#include "gamestate.hpp"
#include "make_move.hpp"
#include "maskset.hpp"
#include "move_logic.hpp"
#include "util.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

#ifdef count_moves

namespace move_counting {

template <int depth_left, GameState state, bool print = false>
size_t count_nodes(auto &masks, Board &board);

// Make capture move, count positions and unmake the move.
template<int depth_left, GameState state, PieceType piece, PieceType captured, bool print>
inline size_t count_capture(auto& masks, Board& board, BitBoard from, BitBoard to) {
    capture_move<state.player(), piece, captured>(board, from, to);
    const uint64_t ret = count_nodes<depth_left-1, state.switch_player()>(masks, board);
    unmake_capture_move<state.player(), piece, captured>(board, from, to);

    // If we are printing moves, print the move.
    #ifdef count_moves_print
    if constexpr (print)
        print_movecount(from, to, ret);
    #endif

    return ret;
}

// Make normal move, count positions and unmake the move.
template<int depth_left, GameState state, PieceType piece, bool print>
inline size_t count_plain_move(auto& masks, Board& board, const BitBoard move) {
    move_piece<state.player(), piece>(board, move);
    const uint64_t ret = count_nodes<depth_left-1, state.switch_player()>(masks, board);
    move_piece<state.player(), piece>(board, move);

    // If we are printing moves, print the move.
    #ifdef count_moves_print
    if constexpr (print) {
        const BitBoard from = move & board.get_player_occ<state.player()>();
        print_movecount(from, move & ~from, ret);
    }
    #endif

    return ret;
}

// Count captures for a specific piece type. This is a wrapper for if
// we don't know the captured piece type at compile time.
template<int depth_left, GameState state, PieceType piece, bool print>
inline size_t count_capture(auto& masks, Board& board, const BitBoard from, const BitBoard to) {
    if (board.get_piece_board<opponent<state.player()>(), PieceType::pawn>() & to) 
        return count_capture<depth_left, state, piece, PieceType::pawn, print>(masks, board, from, to);
    else if (board.get_piece_board<opponent<state.player()>(), PieceType::knight>() & to) 
        return count_capture<depth_left, state, piece, PieceType::knight, print>(masks, board, from, to);
    else if (board.get_piece_board<opponent<state.player()>(), PieceType::bishop>() & to) 
        return count_capture<depth_left, state, piece, PieceType::bishop, print>(masks, board, from, to);
    else if (board.get_piece_board<opponent<state.player()>(), PieceType::rook>() & to) 
        return count_capture<depth_left, state, piece, PieceType::rook, print>(masks, board, from, to);
    else if (board.get_piece_board<opponent<state.player()>(), PieceType::queen>() & to) 
        return count_capture<depth_left, state, piece, PieceType::queen, print>(masks, board, from, to);
    else if (board.get_piece_board<opponent<state.player()>(), PieceType::king>() & to)  {
        board.print_board();
        state.print_state_data();
        throw std::runtime_error("Cannot capture a king. This should never happen.");
    }
    throw std::runtime_error("No piece found to capture.");
}

// Given an attack board, count all the captures for a specific piece type.
template<int depth_left, GameState state, PieceType piece, bool print>
inline size_t count_all_captures(auto& masks, Board& board, BitBoard attack_board, const BitBoard from) {
    if constexpr (depth_left == 1 && piece != PieceType::king && !print) return pcnt(attack_board);
    size_t ret = 0;
    while(attack_board) {
        BitBoard to = pop_extract(attack_board);
        if constexpr(piece == PieceType::king) {
            if (board.is_attacked<state.player()>(to)) continue;
            else if constexpr (!depth_left) {
                ret++;
                continue;
            }
        }
        ret += count_capture<depth_left, state, piece, print>(masks, board, from, to);
    }
    return ret;
}

// Count all the moves for a specific piece type, excluding captures.
template <int depth_left, GameState state, PieceType piece, bool print>
inline size_t count_all_plain_moves(auto& masks, Board& board, BitBoard attack_board, const BitBoard from) {
    // If we are at the last depth, we can just return the number of moves.
    if constexpr (depth_left == 1 && piece != PieceType::king && !print) return pcnt(attack_board);
    size_t ret = 0;
    while(attack_board) {
        BitBoard to = pop_extract(attack_board);
        if constexpr(piece == PieceType::king) {
            if (board.is_attacked<state.player()>(to)) continue;
            else if constexpr (!depth_left) {
                ret++;
                continue;
            }
        }
        // Make the move, count moves from there, undo move.
        ret += count_plain_move<depth_left, state, piece, print>(masks, board, from | to);
    }
    return ret;
}

template <int depth_left, GameState state, PieceType piece, bool print>
inline size_t count_plain_and_captures(auto& masks, Board& board, BitBoard piece_board, BitBoard move_mask) {
    MaskSet& mask = masks[depth_left];
    // If we are counting moves for the king, we can assume there is only one piece.
    if constexpr (piece == PieceType::king) {
        BitBoard attacks = get_king_move(lbit(piece_board)) & move_mask;
        return  count_all_plain_moves<depth_left, state, piece, print>(masks, board, attacks & !board.occ_board, piece_board) +
               count_all_captures<depth_left, state, piece, print>(masks, board, attacks & board.get_player_occ<opponent<state.player()>()>(), piece_board);
    } else {
        // Count moves for all instances of the piece type.
        size_t ret = 0;
        while(piece_board) {
            BitBoard from = pop_extract(piece_board), attacks;
            if constexpr (piece == PieceType::knight) 
                attacks = get_knight_move(lbit(from)) & move_mask;
            else if constexpr (piece == PieceType::pawn) 
                attacks = get_pawn_attacks<state.player()>(from) & move_mask;
            else 
                attacks = get_slider_move<piece>(lbit_from_right(from), board.occ_board) & move_mask;
            
            if (!attacks) continue; 

            print_bitboard(from);
            print_bitboard(attacks);

            ret += count_all_plain_moves<depth_left, state, piece, print>(masks, board, attacks & ~board.get_player_occ<opponent<state.player()>()>(), from);
            ret += count_all_captures<depth_left, state, piece, print>(masks, board, attacks & board.get_player_occ<opponent<state.player()>()>(), from);
        } 
        return ret;
    }
}

// Count all the slider moves.
template <int depth_left, GameState state, bool print>
inline size_t count_slider_moves(auto& masks, Board& board) {
    MaskSet& mask = masks[depth_left];
    // Get slider boards.
	BitBoard bishops = board.get_piece_board<state.player(), PieceType::bishop>();
	BitBoard rooks = board.get_piece_board<state.player(), PieceType::rook>();
	BitBoard queens = board.get_piece_board<state.player(), PieceType::queen>();
    // Seperate pinned from unpinned pieces.
	BitBoard unp_b = bishops & mask.nopin;
	BitBoard unp_r = rooks & mask.nopin;
	BitBoard unp_q = queens & mask.nopin;
    // Pinned squares that the piece can still move to.
	BitBoard pin_cmt_diag = mask.legal_squares & mask.pinmask_dg;
	BitBoard pin_cmt_orth = mask.legal_squares & mask.pinmask_orth;
	BitBoard dg_not_orth = mask.pinmask_dg & ~mask.pinmask_orth;
	BitBoard orth_not_dg = mask.pinmask_orth & ~mask.pinmask_dg;
    // Isolate pinned pieces.
	BitBoard pin_b = bishops & dg_not_orth;
	BitBoard pin_q_diag = queens & dg_not_orth;
	BitBoard pin_r = rooks & orth_not_dg;   
	BitBoard pin_q_orth = queens & orth_not_dg;
	// Pinned + unpinned.
	return count_plain_and_captures<depth_left, state, PieceType::bishop, print>(masks, board, pin_b, pin_cmt_diag)
    + count_plain_and_captures<depth_left, state, PieceType::bishop, print>(masks, board, unp_b, mask.legal_squares)
    + count_plain_and_captures<depth_left, state, PieceType::queen_diagonal, print>(masks, board, pin_q_diag, pin_cmt_diag)
    + count_plain_and_captures<depth_left, state, PieceType::queen, print>(masks, board, unp_q, mask.legal_squares)
    + count_plain_and_captures<depth_left, state, PieceType::queen_orthogonal, print>(masks, board, pin_q_orth, pin_cmt_orth)
    + count_plain_and_captures<depth_left, state, PieceType::rook, print>(masks, board, pin_r, pin_cmt_orth)
    + count_plain_and_captures<depth_left, state, PieceType::rook, print>(masks, board, unp_r, mask.legal_squares);
}

template<int depth_left, GameState state, bool print>
inline size_t count_castling_moves(auto& masks, Board& board) {
    // To do.
    return 0;
}

template<int depth_left, GameState state, bool print>
inline size_t count_en_passant_moves(auto& masks, Board& board) {
    // To do.
    return 0;
}

template<int depth_left, GameState state, bool print>
inline size_t count_pawn_moves(auto& masks, Board& board, BitBoard pawns, BitBoard move_mask) {
    // No moves are possible.
    if(!(move_mask && pawns)) return 0;
    BitBoard captures = move_mask & board.occ_board;
    move_mask ^= captures;
    BitBoard pawns_on_start = pawns & (state.player() == Player::white ? c_promotion_from_b : c_promotion_from_w);
    size_t ret = 0;
    if constexpr (depth_left == 1 && !print) {
        ret += pcnt(get_pawn_double<state.player()>(pawns_on_start, board.occ_board) & move_mask);
        ret += pcnt(get_pawn_forward<state.player()>(pawns) & move_mask);
        return ret + pcnt(get_pawn_attacks<state.player()>(pawns) & captures);
    }
    // Count all the moves for the pawns.
    while(pawns) {
        const BitBoard from = pop_extract(pawns);
        const BitBoard forward = get_pawn_forward<state.player()>(from) & move_mask;
        const BitBoard left_attack = get_pawn_attack<state.player(), true>(from) & captures;
        const BitBoard right_attack = get_pawn_attack<state.player(), false>(from) & captures;
        // For forward moves we can assume only one move is possible.
        if(forward) ret += count_plain_move<depth_left, state, PieceType::pawn, print>(masks, board, from | forward);
        if(left_attack) ret += count_capture<depth_left, state, PieceType::pawn, print>(masks, board, from, left_attack);
        if(right_attack) ret += count_capture<depth_left, state, PieceType::pawn, print>(masks, board, from, right_attack);
    }
    // Count all the double moves.
    while(pawns_on_start) {
        const BitBoard from = pop_extract(pawns_on_start);
        BitBoard attacks = get_pawn_double<state.player()>(from, board.occ_board) & move_mask;
        while(attacks) {
            #ifdef count_moves_print
            size_t old = ret;
            #endif

            BitBoard to = pop_extract(attacks);
            BitBoard move = from | to;
            // Check if en passant is possible next move, if so, set the en_passant mask for next depth.
            if(pawn_double<state.player()>(board, move, to)) {
                masks[depth_left-1].en_passant = to;
                ret += count_nodes<depth_left - 1, state.switch_player().can_ep_next_ply()>(masks, board);
            } 
            else {
                ret += count_nodes<depth_left - 1, state.switch_player()>(masks, board);
            }
            unmake_pawn_double<state.player()>(board, move);

            #ifdef count_moves_print
            if constexpr (print) {
                print_movecount(move & ~to, to, ret - old);
            }
            #endif
        }
    }
    return ret;
}
 
template<int depth_left, GameState state, bool print>
inline size_t count_all_pawn_moves(auto& masks, Board& board) {
    const BitBoard move_mask = masks[depth_left].legal_squares;
    // Seperate promotion pawns, pinned pawns and normal pawns.
    const BitBoard all_pawns = board.get_piece_board<state.player(), PieceType::pawn>();
    const BitBoard promotion_pawns = all_pawns & (state.player() == Player::white ? c_promotion_from_w : c_promotion_from_b);
    BitBoard normal_pawns = all_pawns ^ promotion_pawns;
    const BitBoard pinned_pawns = normal_pawns & ~masks[depth_left].nopin;
    normal_pawns ^= pinned_pawns;
    // Count all the moves for pinned and normal pawns.
    return count_pawn_moves<depth_left, state, print>(masks, board, normal_pawns, move_mask) 
        + count_pawn_moves<depth_left, state, print>(masks, board, pinned_pawns, move_mask ^ masks[depth_left].nopin);
}

template <int depth_left, GameState state, bool print>
inline size_t count_king_moves(auto& masks, Board& board, BitBoard legal_squares) {
    return count_plain_and_captures<depth_left, state.rm_cr<state.player()>(), PieceType::king, print>(
        masks, board, board.get_piece_board<state.player(), PieceType::king>(), legal_squares
    );
}

// Root for counting the moves from an arbitrary position.
// Works by seperating the move types and returning the sum of all the counts.
template <int depth_left, GameState state, bool print>
size_t count_nodes(auto &masks, Board &board) {
    if constexpr (!depth_left) {
        return 1;
    } else {
        // Make masks. These decide the legal squares, pin masks and check masks.
        // Pin mask: the squares that are pinned. Pinned piece movement is limited to these squares.
        // Check mask: the squares that are between the king and (include) the checking piece.
        MaskSet& mask = masks[depth_left];
        mask.create_masks<state.player()>(board);
        // King moves can always be generated.
        size_t ret = 0;
        ret += count_king_moves<depth_left, state, print>(masks, board, mask.legal_squares);
        // If no check, we can count all the moves, including castling.
        // If there is one checker, we can only block the check or capture the piece.
        // If there are two or more checkers, we can only move the king.
        // if(!mask.checkers) ret += count_castling_moves<depth_left, state>(masks, board);
        // else if (mask.checkers == 1) mask.legal_squares &= mask.check_mask;
        // else return ret;
        
        // Count all knight, pawn, and slider moves.
        ret += count_plain_and_captures<depth_left, state, PieceType::knight, print>(
             masks, board, board.get_piece_board<state.player(), PieceType::knight>(), mask.legal_squares
        );
        ret += count_slider_moves<depth_left, state, print>(masks, board);
        ret += count_all_pawn_moves<depth_left, state, print>(masks, board);
        if constexpr (state.can_ep()) ret += count_en_passant_moves<depth_left, state, print>(masks, board);
        
        return ret;
    }
}


// Starting point for counting moves.
template <int depth, GameState state>
static size_t count(Board& board) {
    MaskSet masks[depth + 1];
    // Count moves and time it.
	const clock_t start = clock();
	const size_t nodes = count_nodes<depth, state, true>(masks, board);
	const clock_t end = clock();
    // Print the results.
    std::cout << "================================================================================ \n";
	const double time_cost = double(end - start) / CLOCKS_PER_SEC;
	std::cout << "PERFT results: \nNodes evaluated: " << nodes << "\nTime cost: " << time_cost << '\n';
	std::cout << std::round((nodes / 1'000'000)) / time_cost << " million nodes per second" << '\n';
	std::cout << "================================================================================ \n";
	return nodes;
}

} // namespace move_counting

#endif // count_moves