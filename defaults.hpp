#include <cstdint>

#pragma once

#define run_tests 1

// Define aliases for data structures.
typedef uint64_t BitBoard;
typedef uint_fast8_t Square; 

// Define default chess board configuration.
// Bit on 1 means a piece is initialized on that particular square.
constexpr BitBoard EMPTY_BOARD = 0b0;
constexpr uint64_t INIT_ROOK_SQUARES = 0b1000000100000000000000000000000000000000000000000000000010000001ULL;
constexpr uint64_t INIT_KNIGHT_SQUARES = 0b0100001000000000000000000000000000000000000000000000000001000010ULL;
constexpr uint64_t INIT_BISHOP_SQUARES = 0b0010010000000000000000000000000000000000000000000000000000100100ULL;
constexpr uint64_t INIT_QUEEN_SQUARES = 0b0001000000000000000000000000000000000000000000000000000000010000ULL;
constexpr uint64_t INIT_KING_SQUARES = 0b0000100000000000000000000000000000000000000000000000000000001000ULL;
constexpr uint64_t INIT_PAWN_SQUARES = 0b0000000011111111000000000000000000000000000000001111111100000000ULL;
constexpr uint64_t INIT_BLACK_PIECES = 0b1111111111111111000000000000000000000000000000000000000000000000ULL;
constexpr uint64_t INIT_TOTAL_SQUARES = 0b1111111111111111000000000000000000000000000000001111111111111111ULL;
constexpr uint64_t INIT_WHITE_PIECES = 0b0000000000000000000000000000000000000000000000001111111111111111ULL;
constexpr uint64_t FULL_BOARD = 0xFFFFFFFFFFFFFFFFULL;
 
// When making masks, the edges are irrelevant.
constexpr BitBoard BISHOP_MASK_FILTER = 0b0000000001111110011111100111111001111110011111100111111000000000;

// Define board edges.
constexpr BitBoard LEFT_EDGE =  0b1000000010000000100000001000000010000000100000001000000010000000ULL;
constexpr BitBoard RIGHT_EDGE = 0b0000000100000001000000010000000100000001000000010000000100000001ULL;
constexpr BitBoard TOP_RANK =   0b1111111100000000000000000000000000000000000000000000000000000000ULL;
constexpr BitBoard BOTTOM_RANK = 0b000000000000000000000000000000000000000000000000000000011111111ULL;

// Board size.
constexpr int BOARD_SIZE = 64;

// Max possible permutations for a mask.
constexpr int MAX_PERM_ROOK = 4096;
constexpr int MAX_PERM_BISH = 512;

// Maximum magic number value.
// This is used to ensure that the magic number is not too small, which would lead to
// collisions in the magic number hashing function.
constexpr uint64_t MIN_MAGIC_VALUE = 0xFF00000000000000ULL;

// Relevant bits for magic bitboards.
constexpr int REL_ROOK_BITS[64] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};

constexpr int REL_BISH_BITS[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

// Maximum attempts to generate a magic number before raising an error.
constexpr int MAX_MAGIC_ATTEMPTS = 10000000;

// Define slider types, this can be used with 
// template specialization to create different attack tables.
enum class SliderType {
    diagonal,
    orthogonal
};

// Fast check to see if slider type is diagonal.
#define IS_DIAGONAL(slider_type) (slider_type == SliderType::diagonal)

// Shorten builtin function names.
#define pcnt __builtin_popcountll

#ifdef __BMI2__
#define pext _pdep_u64
#endif