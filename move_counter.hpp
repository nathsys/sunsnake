#include "defaults.hpp"
#include "gamestate.hpp"
#include "make_move.hpp"
#include "maskset.hpp"
#include "move_logic.hpp"
#include "util.hpp"
#include "movelist.hpp"
#include "move.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

#ifdef count_moves

namespace move_counting
{
    // Return a capture move.
    inline Move get_capture(Square from, Square to)
    {
        return MoveEncoding::capture(from, to);
    }

    // Return a quiet move.
    inline Move get_plain(Square from, Square to)
    {
        return MoveEncoding::quiet_move(from, to);
    }

    // Given an attack board and a specific square, get all the captures for a specific piece type.
    template <PieceType piece, bool capture>
    inline void moves_for_piece(BitBoard attack_board, const Square from, MoveStack &moves)
    {
        while (attack_board)
        {
            Square to = pop(attack_board);
            moves.push(capture ? get_capture(from, to) : get_plain(from, to));
        }
    }

    // Count all the moves (plain + captures) for a specific piece type.
    template <PieceType piece, Player player, SliderType slider_type = SliderType::none>
    inline void moves_for_piecetype(Board &board, BitBoard piece_board, BitBoard move_mask, MoveStack &moves)
    {
        // For each piece instance, get the moves.
        while (piece_board)
        {
            BitBoard attacks = 0b0;
            Square from = pop(piece_board);
            if constexpr (piece == PieceType::knight)
                attacks = get_knight_move(from) & move_mask;
            else if constexpr (piece == PieceType::pawn)
                attacks = get_pawn_attacks<player>(square_to_mask(from)) & move_mask;
            else
                attacks = get_slider_move<piece, slider_type>(from, board.occ_board) & move_mask;

            if (!attacks)
                continue;

            moves_for_piece<piece, false>(attacks & ~board.occ_board, from, moves);
            moves_for_piece<piece, true>(attacks & board.get_player_occ<opponent<player>()>(), from, moves);
        }
    }

    // Count all the slider moves.
    template <Player player>
    inline void get_slider_moves(MaskSet &mask, Board &board, MoveStack &moves)
    {
        // Get slider boards.
        BitBoard bishops = board.get_piece_board<player, PieceType::bishop>();
        BitBoard rooks = board.get_piece_board<player, PieceType::rook>();
        BitBoard queens = board.get_piece_board<player, PieceType::queen>();
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
        moves_for_piecetype<PieceType::bishop, player>(board, pin_b, pin_cmt_diag, moves);
        moves_for_piecetype<PieceType::bishop, player>(board, unp_b, mask.legal_squares, moves);
        moves_for_piecetype<PieceType::queen, player, SliderType::diagonal>(board, pin_q_diag, pin_cmt_diag, moves);
        moves_for_piecetype<PieceType::queen, player>(board, unp_q, mask.legal_squares, moves);
        moves_for_piecetype<PieceType::queen, player, SliderType::orthogonal>(board, pin_q_orth, pin_cmt_orth, moves);
        moves_for_piecetype<PieceType::rook, player>(board, pin_r, pin_cmt_orth, moves);
        moves_for_piecetype<PieceType::rook, player>(board, unp_r, mask.legal_squares, moves);
    }

    inline void count_castling_moves()
    {
        // To do.
        return;
    }

    inline void count_en_passant_moves()
    {
        // To do.
        return;
    }

    template <Player player>
    inline void count_pawn_moves(BitBoard occ_board, BitBoard pawns, BitBoard move_mask, MoveStack &moves)
    {
        // No moves are possible.
        if (!(move_mask && pawns))
            return;
        // Remove all the captures from the move mask.
        BitBoard captures = move_mask & occ_board;
        move_mask ^= captures;
        BitBoard pawns_on_start = pawns & (player == Player::white ? c_promotion_from_b : c_promotion_from_w);

        // Count all the normal moves for the pawns.
        while (pawns)
        {
            const BitBoard from = pop_extract(pawns);
            const Square from_sq = lbit_from_right(from);
            const BitBoard forward = get_pawn_forward<player>(from) & move_mask;
            const BitBoard left_attack = get_pawn_attack<player, true>(from) & captures;
            const BitBoard right_attack = get_pawn_attack<player, false>(from) & captures;
            if (forward)
                moves.push(get_plain(from_sq, lbit_from_right(forward)));
            if (left_attack)
                moves.push(get_capture(from_sq, lbit_from_right(left_attack)));
            if (right_attack)
                moves.push(get_capture(from_sq, lbit_from_right(right_attack)));
        }

        // Count all the double moves.
        while (pawns_on_start)
        {
            const BitBoard from = pop_extract(pawns_on_start);
            const Square from_sq = lbit_from_right(from);
            BitBoard attacks = get_pawn_double<player>(from, occ_board) & move_mask;
            while (attacks)
            {
                const BitBoard to = pop_extract(attacks);
                moves.push(MoveEncoding::double_pawn_push(from_sq, lbit_from_right(to)));
            }
        }
    }

    template <Player player>
    inline void count_all_pawn_moves(Board &board, BitBoard move_mask, BitBoard not_pinned, MoveStack &moves)
    {
        // Seperate promotion pawns, pinned pawns and normal pawns.
        const BitBoard all_pawns = board.get_piece_board<player, PieceType::pawn>();
        const BitBoard occ_board = board.occ_board;
        const BitBoard promotion_pawns = all_pawns & (player == Player::white ? c_promotion_from_w : c_promotion_from_b);
        BitBoard normal_pawns = all_pawns ^ promotion_pawns;
        const BitBoard pinned_pawns = normal_pawns & ~not_pinned;
        normal_pawns ^= pinned_pawns;
        // Get all the moves for pinned and normal pawns.
        count_pawn_moves<player>(occ_board, normal_pawns, move_mask, moves);
        count_pawn_moves<player>(occ_board, pinned_pawns, move_mask ^ not_pinned, moves);
    }

    template <Player player>
    inline void get_king_moves(Board &board, BitBoard piece_board, BitBoard move_mask, MoveStack &moves)
    {
        Square from_sq = lbit_from_right(piece_board);
        BitBoard attacks = get_king_move(from_sq) & move_mask;
        moves_for_piece<PieceType::king, false>(attacks & ~board.occ_board, from_sq, moves);
        moves_for_piece<PieceType::king, true>(attacks & board.get_player_occ<opponent<player>()>(), from_sq, moves);
    }

    template <Player player>
    MoveList get_moves(Board &board, GameState state, MoveStack &move_stack)
    {
        Move *top = &move_stack.top();
        // Make masks. These decide the legal squares, pin masks and check masks.
        // Pin mask: the squares that are pinned. Pinned piece movement is limited to these squares.
        // Check mask: the squares that are between the king and (include) the checking piece.
        MaskSet mask;
        mask.create_masks<player>(board);
        // King moves can always be generated.
        get_king_moves<player>(board, board.get_piece_board<player, PieceType::king>(), mask.legal_squares, move_stack);
        // If no check, we can count all the moves, including castling.
        // If there is one checker, we can only block the check or capture the piece.
        // If there are two or more checkers, we can only move the king.
        if (!mask.checkers)
            count_castling_moves();
        else if (mask.checkers == 1)
            mask.legal_squares &= mask.check_mask;
        else
            return move_stack.from(top);

        // Get all knight, pawn, and slider moves.
        moves_for_piecetype<PieceType::knight, player>(board, board.get_piece_board<player, PieceType::knight>(), mask.legal_squares, move_stack);
        get_slider_moves<player>(mask, board, move_stack);
        count_all_pawn_moves<player>(board, mask.legal_squares, mask.nopin, move_stack);
        if (state.can_ep())
            count_en_passant_moves();

        return move_stack.from(top);
    }

    template <Player player, bool is_root = false>
    static size_t perft(Board &board, GameState &state, int depth, MoveStack &move_stack, FlagStack &data_stack)
    {
        size_t nodes = 0;
        if (depth == 1 && !is_root)
            return get_moves<player>(board, state, move_stack).size();
        if (!depth)
            return 1;
        for (Move &move : get_moves<player>(board, state, move_stack))
        {
            make_move::make_move<player>(board, state, move, data_stack);
            size_t local = perft<opponent<player>()>(board, state, depth - 1, move_stack, data_stack);
            make_move::unmake_move<player>(board, state, move, data_stack);

            if constexpr (is_root)
                print_movecount(move, local);

            nodes += local;
        }
        if constexpr (is_root)
            std::cout << "\nNodes searched: " << nodes << '\n';
        return nodes;
    }

    // Parses the depth to a template parameter and calls the templated count function.
    static void perft(Board &board, GameState state, int depth)
    {
        size_t nodes;
        MoveStack move_stack;
        FlagStack data_stack;
        if (state.player() == Player::white)
            nodes = perft<Player::white, true>(board, state, depth, move_stack, data_stack);
        else
            nodes = perft<Player::black, true>(board, state, depth, move_stack, data_stack);
    }

} // namespace move_counting

#endif // count_moves