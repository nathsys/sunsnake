#include "defaults.hpp"

#ifndef move_hpp
#define move_hpp

typedef uint16_t Move;
typedef uint8_t MoveFlag;

namespace MoveEncoding
{

    // Move data layout:
    // 1111 11.. .... .... <- from square (6 bits)
    // .... ..11 1111 .... <- to square (6 bits)
    // .... .... .... 1111 <- flags (4 bits)
    // Flags:
    // 0000 - quiet move
    constexpr MoveFlag quiet_flag = 0b0000;
    // 0001 - double pawn push
    constexpr MoveFlag double_pawn_push_flag = 0b0001;
    // 0010 - kingside castle
    constexpr MoveFlag kingside_castle_flag = 0b0010;
    // 0011 - queenside castle
    constexpr MoveFlag queenside_castle_flag = 0b0011;
    // 0100 - captures
    constexpr MoveFlag capture_flag = 0b0100;
    // 0101 - ep-capture
    constexpr MoveFlag ep_capture_flag = 0b0101;
    // 1000 - knight promotion
    constexpr MoveFlag knight_promotion_flag = 0b1000;
    // 1001 - bishop promotion
    constexpr MoveFlag bishop_promotion_flag = 0b1001;
    // 1010 - rook promotion
    constexpr MoveFlag rook_promotion_flag = 0b1010;
    // 1011 - queen promotion
    constexpr MoveFlag queen_promotion_flag = 0b1011;
    // 1100 - knight promotion capture
    constexpr MoveFlag knight_promotion_capture_flag = 0b1100;
    // 1101 - bishop promotion capture
    constexpr MoveFlag bishop_promotion_capture_flag = 0b1101;
    // 1110 - rook promotion capture
    constexpr MoveFlag rook_promotion_capture_flag = 0b1110;
    // 1111 - queen promotion capture
    constexpr MoveFlag queen_promotion_capture_flag = 0b1111;

    static Move flagless_move(Square from, Square to)
    {
        return (static_cast<Move>(from) << 10) | static_cast<Move>(to << 4);
    }

    static Move quiet_move(Square from, Square to)
    {
        return quiet_flag | flagless_move(from, to);
    }

    static Move double_pawn_push(Square from, Square to)
    {
        return double_pawn_push_flag | flagless_move(from, to);
    }

    static Move kingside_castle(Square from, Square to)
    {
        return kingside_castle_flag | flagless_move(from, to);
    }

    static Move queenside_castle(Square from, Square to)
    {
        return queenside_castle_flag | flagless_move(from, to);
    }

    static Move capture(Square from, Square to)
    {
        return capture_flag | flagless_move(from, to);
    }

    static Move ep_capture(Square from, Square to)
    {
        return ep_capture_flag | flagless_move(from, to);
    }

    template <PieceType promotion_piece>
    static Move promotion(Square from, Square to, bool is_capture)
    {
        MoveFlag flag = 0;
        switch (promotion_piece)
        {
        case PieceType::knight:
            flag = is_capture ? knight_promotion_capture_flag : knight_promotion_flag;
            break;
        case PieceType::bishop:
            flag = is_capture ? bishop_promotion_capture_flag : bishop_promotion_flag;
            break;
        case PieceType::rook:
            flag = is_capture ? rook_promotion_capture_flag : rook_promotion_flag;
            break;
        case PieceType::queen:
            flag = is_capture ? queen_promotion_capture_flag : queen_promotion_flag;
            break;
        default:
            // Invalid promotion piece
            return 0;
        }
        return flag | flagless_move(from, to);
    }

    static MoveFlag get_move_flag(Move move)
    {
        return move & 0b1111;
    }

    static Square get_from_square(Move move)
    {
        return (move >> 10) & 0b111111;
    }

    static Square get_to_square(Move move)
    {
        return (move >> 4) & 0b111111;
    }

    static inline std::string make_chess_notation(const Square square)
    {
        // Convert column index (0-7) to letter (a-h)
        char column = 'h' - (square % 8);
        // Convert row index (0-7) to number (1-8)
        char row = '1' + (square / 8);
        return std::string(1, column) + std::string(1, row);
    }

    static std::string move_to_string(Move move)
    {
        Square from = get_from_square(move);
        Square to = get_to_square(move);
        MoveFlag flag = get_move_flag(move);

        std::string move_str = make_chess_notation(from) + make_chess_notation(to);

        switch (flag)
        {
        case knight_promotion_flag:
        case knight_promotion_capture_flag:
            move_str += 'n';
            break;
        case bishop_promotion_flag:
        case bishop_promotion_capture_flag:
            move_str += 'b';
            break;
        case rook_promotion_flag:
        case rook_promotion_capture_flag:
            move_str += 'r';
            break;
        case queen_promotion_flag:
        case queen_promotion_capture_flag:
            move_str += 'q';
            break;
        default:
            break;
        }

        return move_str;
    }
}

#endif // move_hpp