#include <cstdint>
#include "movelist.hpp"

#pragma once

#define run_tests 1
#define count_moves 1
#define count_moves_print 1

// Define aliases for data structures.
typedef uint64_t BitBoard;
typedef uint_fast8_t Square;

// Define default chess board configuration.
// Bit on 1 means a piece is initialized on that particular square.
constexpr BitBoard c_empty_board = 0b0;
constexpr uint64_t c_init_rook_squares = 0b1000000100000000000000000000000000000000000000000000000010000001ULL;
constexpr uint64_t c_init_knight_squares = 0b0100001000000000000000000000000000000000000000000000000001000010ULL;
constexpr uint64_t c_init_bishop_squares = 0b0010010000000000000000000000000000000000000000000000000000100100ULL;
constexpr uint64_t c_init_queen_squares = 0b0001000000000000000000000000000000000000000000000000000000010000ULL;
constexpr uint64_t c_init_king_squares = 0b0000100000000000000000000000000000000000000000000000000000001000ULL;
constexpr uint64_t c_init_pawn_squares = 0b0000000011111111000000000000000000000000000000001111111100000000ULL;
constexpr uint64_t c_init_black_pieces = 0b1111111111111111000000000000000000000000000000000000000000000000ULL;
constexpr uint64_t c_init_total_squares = 0b1111111111111111000000000000000000000000000000001111111111111111ULL;
constexpr uint64_t c_init_white_pieces = 0b0000000000000000000000000000000000000000000000001111111111111111ULL;
constexpr uint64_t c_full_board = 0xFFFFFFFFFFFFFFFFULL;

// When making masks, the edges are irrelevant.
constexpr BitBoard c_bishop_mask_filter = 0b0000000001111110011111100111111001111110011111100111111000000000;

// Define board edges.
constexpr BitBoard c_left_edge = 0b1000000010000000100000001000000010000000100000001000000010000000ULL;
constexpr BitBoard c_right_edge = 0b0000000100000001000000010000000100000001000000010000000100000001ULL;
constexpr BitBoard c_top_rank = 0b1111111100000000000000000000000000000000000000000000000000000000ULL;
constexpr BitBoard c_bottom_rank = 0b000000000000000000000000000000000000000000000000000000011111111ULL;
constexpr BitBoard c_no_left_edge = ~c_left_edge;
constexpr BitBoard c_no_right_edge = ~c_right_edge;
// Promotion squares.
constexpr BitBoard c_promotion_from_w = c_init_pawn_squares & c_init_black_pieces;
constexpr BitBoard c_promotion_from_b = c_init_pawn_squares & c_init_white_pieces;
// En passant squares
constexpr BitBoard c_en_passant_squares_w = c_promotion_from_b << 16;
constexpr BitBoard c_en_passant_squares_b = c_promotion_from_w >> 16;

// Board size.
constexpr int c_board_size = 64;

// Max possible permutations for a mask.
constexpr int c_max_perm_rook = 4096;
constexpr int c_max_perm_bish = 512;

// Maximum magic number value.
// This is used to ensure that the magic number is not too small, which would lead to
// collisions in the magic number hashing function.
constexpr uint64_t c_min_magic_value = 0xFF00000000000000ULL;

// Relevant bits for magic bitboards.
constexpr int c_rel_rook_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12};

constexpr int c_rel_bish_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6};

constexpr int c_max_magic_attempts = 10000000;

// Define slider types, this can be used with
// template specialization to create different attack tables.
enum class SliderType
{
  diagonal,
  orthogonal,
  none
};

// Define players/colors.
enum class Player
{
  white,
  black
};

// Define piece types.
enum class PieceType
{
  pawn,
  knight,
  bishop,
  rook,
  queen,
  king,
  none
};

// Maximum depth for move counting.
constexpr int c_max_count_depth = 10;

// Fast check to see if slider type is diagonal.
#define IS_DIAGONAL(slider_type) (slider_type == SliderType::diagonal)

// Shorten builtin function names.
#define pcnt __builtin_popcountll

#define STACKSIZE 256
#define MoveStack Stack<Move, STACKSIZE>
#define MoveList ListFromStack<Move, STACKSIZE>
#define FlagStack Stack<u_int8_t, STACKSIZE>

#ifdef __BMI2__
#define pdep _pdep_u64
#define pext _pext_u64
#endif