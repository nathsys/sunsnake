#include "defaults.hpp"
#include "gamestate.hpp"
#include "make_move.hpp"
#include "maskset.hpp"
#include "move_logic.hpp"
#include "util.hpp"
#include "movelist.hpp"
#include "move.hpp"

#pragma once

#ifdef move_gen

namespace move_generation
{
    // Given an attack board and a specific square, get all the captures for a specific piece type.
    template <PieceType piece, Player player, bool capture>
    inline void moves_for_piece(Board &board, BitBoard attack_board, const Square from, MoveStack &moves)
    {
        while (attack_board)
        {
            Square to = pop(attack_board);
            if constexpr (piece == PieceType::king)
            {
                if (board.is_attacked<player>(to))
                    continue;
            }
            moves.push(capture ? MoveEncoding::capture(from, to) : MoveEncoding::quiet_move(from, to));
        }
    }

    // Generate all the moves (plain + captures) for a specific piece type.
    template <PieceType piece, Player player, SliderType slider_override = SliderType::none>
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
                attacks = get_slider_move<piece, slider_override>(from, board.occ_board) & move_mask;

            if (!attacks)
                continue;

            moves_for_piece<piece, player, false>(board, attacks & ~board.occ_board, from, moves);
            moves_for_piece<piece, player, true>(board, attacks & board.get_player_occ<opponent<player>()>(), from, moves);
        }
    }

    // Generate all the slider moves.
    template <Player player>
    inline void get_slider_moves(MaskSet &mask, Board &board, MoveStack &moves)
    {
        BitBoard bishops = board.get_piece_board<player, PieceType::bishop>();
        BitBoard rooks = board.get_piece_board<player, PieceType::rook>();
        BitBoard queens = board.get_piece_board<player, PieceType::queen>();
        const PinSplit pins = mask.split_pins();
        // Pinned + unpinned.
        moves_for_piecetype<PieceType::bishop, player>(board, bishops & pins.dg_not_orth, pins.cmt_diag, moves);
        moves_for_piecetype<PieceType::bishop, player>(board, bishops & mask.nopin, mask.legal_squares, moves);
        moves_for_piecetype<PieceType::queen, player, SliderType::diagonal>(board, queens & pins.dg_not_orth, pins.cmt_diag, moves);
        moves_for_piecetype<PieceType::queen, player>(board, queens & mask.nopin, mask.legal_squares, moves);
        moves_for_piecetype<PieceType::queen, player, SliderType::orthogonal>(board, queens & pins.orth_not_dg, pins.cmt_orth, moves);
        moves_for_piecetype<PieceType::rook, player>(board, rooks & pins.orth_not_dg, pins.cmt_orth, moves);
        moves_for_piecetype<PieceType::rook, player>(board, rooks & mask.nopin, mask.legal_squares, moves);
    }

    template <Player player, bool kingside>
    inline void generate_castling_moves(const Board &board, const MaskSet &mask, const GameState &state, MoveStack &moves)
    {
        if (!(kingside ? state.can_castle_k(player) : state.can_castle_q(player)))
            return;

        const BitBoard rook_mask = board.get_piece_board<player, PieceType::rook>() & get_corner<player, kingside>();
        if (!rook_mask)
            return;
        const BitBoard king_path = Lookup::between_squares[mask.king_square][lbr(rook_mask)] & ~rook_mask;

        if (king_path & board.occ_board)
            return;
        if (!board.castle_sqs_attacked<player, kingside>())
        {
            moves.push(MoveEncoding::castle<player, kingside>());
        }
    }

    template <Player player>
    inline void generate_en_passant_moves(Board &board, MaskSet &mask, GameState &state, MoveStack &moves)
    {
        // Simulate en passant moves and see if king is in check after the move.
        const Square ep_target = state.ep_target<player>();
        const BitBoard ep_target_board = square_to_mask(ep_target);
        const BitBoard capture_pawn = player == Player::white ? ep_target_board >> 8 : ep_target_board << 8;
        BitBoard adj_pawns = board.get_piece_board<player, PieceType::pawn>() & (capture_pawn << 1 | capture_pawn >> 1);
        adj_pawns &= player == Player::white ? c_en_passant_squares_b : c_en_passant_squares_w;
        while (adj_pawns)
        {
            const Square from = pop(adj_pawns);
            make_move::ep_capture<player>(board, from, ep_target);
            if (!board.is_attacked_by_slider<player>(board.get_piece_board<player, PieceType::king>()))
                moves.push(MoveEncoding::ep_capture(from, ep_target));

            make_move::unmake_ep_capture<player>(board, from, ep_target);
        }
    }

    template <Player player>
    inline void generate_pawn_moves(Board &board, BitBoard pawns, BitBoard move_mask, MoveStack &moves)
    {
        // No moves are possible.
        if (!(move_mask && pawns))
            return;
        // Get enemy pieces that can be captured
        BitBoard enemy_pieces = move_mask & board.get_player_occ<opponent<player>()>();
        // Remove all occupied squares from forward move mask
        move_mask &= ~board.occ_board;
        BitBoard pawns_on_start = pawns & (player == Player::white ? c_promotion_from_b : c_promotion_from_w);

        // Generate all the normal moves for the pawns.
        while (pawns)
        {
            const BitBoard from = pop_extract(pawns);
            const Square from_sq = lbr(from);
            const auto [forward, left_attack, right_attack] = get_pawn_targets<player>(from, move_mask, enemy_pieces);
            if (forward)
                moves.push(MoveEncoding::quiet_move(from_sq, lbr(forward)));
            if (left_attack)
                moves.push(MoveEncoding::capture(from_sq, lbr(left_attack)));
            if (right_attack)
                moves.push(MoveEncoding::capture(from_sq, lbr(right_attack)));
        }

        // Generate all the double moves.
        while (pawns_on_start)
        {
            const BitBoard from = pop_extract(pawns_on_start);
            const Square from_sq = lbr(from);
            BitBoard attacks = get_pawn_double<player>(from, board.occ_board) & move_mask;
            while (attacks)
            {
                const BitBoard to = pop_extract(attacks);
                moves.push(MoveEncoding::double_pawn_push(from_sq, lbr(to)));
            }
        }
    }

    inline void push_promotions(MoveStack &moves, Square from, Square to, bool is_capture)
    {
        moves.push(MoveEncoding::promotion<PieceType::queen>(from, to, is_capture));
        moves.push(MoveEncoding::promotion<PieceType::rook>(from, to, is_capture));
        moves.push(MoveEncoding::promotion<PieceType::bishop>(from, to, is_capture));
        moves.push(MoveEncoding::promotion<PieceType::knight>(from, to, is_capture));
    }

    template <Player player>
    inline void generate_promotion_moves(Board &board, BitBoard pawns, BitBoard move_mask, MoveStack &moves)
    {
        if (!(move_mask && pawns))
            return;
        BitBoard enemy_pieces = move_mask & board.get_player_occ<opponent<player>()>();
        BitBoard push_mask = move_mask & ~board.occ_board;

        while (pawns)
        {
            const BitBoard from = pop_extract(pawns);
            const Square from_sq = lbr(from);
            const auto [forward, left_attack, right_attack] = get_pawn_targets<player>(from, push_mask, enemy_pieces);
            if (forward)
                push_promotions(moves, from_sq, lbr(forward), false);
            if (left_attack)
                push_promotions(moves, from_sq, lbr(left_attack), true);
            if (right_attack)
                push_promotions(moves, from_sq, lbr(right_attack), true);
        }
    }

    template <Player player>
    inline void generate_all_pawn_moves(Board &board, MaskSet &mask, MoveStack &moves)
    {
        // Seperate promotion pawns, pinned pawns and normal pawns.
        const BitBoard all_pawns = board.get_piece_board<player, PieceType::pawn>();
        BitBoard normal_promotion_pawns = all_pawns & (player == Player::white ? c_promotion_from_w : c_promotion_from_b);
        BitBoard normal_pawns = all_pawns ^ normal_promotion_pawns;
        const BitBoard pinned_pawns = extract_pinned(normal_pawns, mask.nopin);
        const BitBoard pinned_promotion_pawns = extract_pinned(normal_promotion_pawns, mask.nopin);
        // A pinned pawn must stay on the specific line pinning it. A pawn
        // pinned diagonally can't use an unrelated orthogonal pin's squares
        // (and vice versa), so each pin type must be handled separately.
        const PinSplit pins = mask.split_pins();
        generate_pawn_moves<player>(board, normal_pawns, mask.legal_squares, moves);
        generate_pawn_moves<player>(board, pinned_pawns & pins.dg_not_orth, pins.cmt_diag, moves);
        generate_pawn_moves<player>(board, pinned_pawns & pins.orth_not_dg, pins.cmt_orth, moves);
        generate_promotion_moves<player>(board, normal_promotion_pawns, mask.legal_squares, moves);
        generate_promotion_moves<player>(board, pinned_promotion_pawns & pins.dg_not_orth, pins.cmt_diag, moves);
        generate_promotion_moves<player>(board, pinned_promotion_pawns & pins.orth_not_dg, pins.cmt_orth, moves);
    }

    template <Player player>
    inline void get_king_moves(Board &board, MaskSet &mask, MoveStack &moves)
    {
        Square from_sq = mask.king_square;
        BitBoard attacks = get_king_move(from_sq) & mask.legal_squares;
        BitBoard king_mask = board.get_piece_board<player, PieceType::king>();
        board.occ_board ^= king_mask;
        moves_for_piece<PieceType::king, player, false>(board, attacks & ~board.occ_board, from_sq, moves);
        moves_for_piece<PieceType::king, player, true>(board, attacks & board.get_player_occ<opponent<player>()>(), from_sq, moves);
        board.occ_board ^= king_mask;
    }

    template <Player player>
    MoveList generate_moves(Board &board, GameState state, MoveStack &move_stack)
    {
        Move *top = &move_stack.top();
        // Make masks. These decide the legal squares, pin masks and check masks.
        // Pin mask: the squares that are pinned. Pinned piece movement is limited to these squares.
        // Check mask: the squares that are between the king and (include) the checking piece.
        MaskSet mask;
        mask.create_masks<player>(board);
        // King moves can always be generated.
        get_king_moves<player>(board, mask, move_stack);
        // If no check, we can generate all the moves, including castling.
        // If there is one checker, we can only block the check or capture the piece.
        // If there are two or more checkers, we can only move the king.
        if (!mask.checkers)
        {
            generate_castling_moves<player, true>(board, mask, state, move_stack);
            generate_castling_moves<player, false>(board, mask, state, move_stack);
        }
        else if (mask.checkers == 1)
        {
            mask.legal_squares &= mask.check_mask;
        }
        else
            return move_stack.from(top);

        // Get all knight, pawn, and slider moves.
        moves_for_piecetype<PieceType::knight, player>(board, board.get_piece_board<player, PieceType::knight>() & mask.nopin, mask.legal_squares, move_stack);
        get_slider_moves<player>(mask, board, move_stack);
        generate_all_pawn_moves<player>(board, mask, move_stack);
        if (state.can_ep())
            generate_en_passant_moves<player>(board, mask, state, move_stack);

        return move_stack.from(top);
    }

} // namespace move_generation

#endif // move_gen
