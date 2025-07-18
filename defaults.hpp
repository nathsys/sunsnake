#include <cstdint>

#pragma once

// Define pieces. Note that we seperate diagonal and orthogonal for the queen so that we can reuse 
// bishop and rook functions.
#define PAWN	   1
#define KING	   2
#define ROOK	   3
#define BISHOP	   4
#define KNIGHT	   5
#define QUEEN	   6
#define QUEEN_DIAG 7
#define QUEEN_ORTH 8
#define EMPTY	   0

// Define aliases for data structures.
typedef uint64_t BitBoard;
typedef uint_fast8_t Square; 

// Define default chess board configuration.
// Bit on 1 means a piece is initialized on that particular square.
const BitBoard EMPTY_BOARD = 0b0;
const uint64_t INIT_ROOK_SQUARES = 0b1000000100000000000000000000000000000000000000000000000010000001ULL;
const uint64_t INIT_KNIGHT_SQUARES = 0b0100001000000000000000000000000000000000000000000000000001000010ULL;
const uint64_t INIT_BISHOP_SQUARES = 0b0010010000000000000000000000000000000000000000000000000000100100ULL;
const uint64_t INIT_QUEEN_SQUARES = 0b0001000000000000000000000000000000000000000000000000000000010000ULL;
const uint64_t INIT_KING_SQUARES = 0b0000100000000000000000000000000000000000000000000000000000001000ULL;
const uint64_t INIT_PAWN_SQUARES = 0b0000000011111111000000000000000000000000000000001111111100000000ULL;
const uint64_t INIT_BLACK_PIECES = 0b1111111111111111000000000000000000000000000000000000000000000000ULL;
const uint64_t INIT_TOTAL_SQUARES = 0b1111111111111111000000000000000000000000000000001111111111111111ULL;
const uint64_t INIT_WHITE_PIECES = 0b0000000000000000000000000000000000000000000000001111111111111111ULL;
 
// When making masks, the edges are irrelevant.
const uint64_t MASK_SQUARES = 0b0000000001111110011111100111111001111110011111100111111000000000;

// Board size.
const int BOARD_SIZE = 64;

// Max possible permutations for a mask.
constexpr int MAX_PERM_ROOK = 4096;
constexpr int MAX_PERM_BISH = 512;

// Relevant bits for magic bitboards.
const int REL_ROOK_BITS[64] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};

const int REL_BISH_BITS[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

// Shorten builtin function names.
#define pcnt __builtin_popcountll
