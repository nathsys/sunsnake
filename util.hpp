#include <cstdio>
#include <iostream>
#include <random>
#include <string>

#include "defaults.hpp"

#pragma once

inline bool get_bit_64(uint64_t num, uint8_t pos) { return (num & (1ULL << (pos))) != 0; }

inline constexpr uint64_t square_to_mask(Square s) { return 1ULL << s; }

inline uint8_t lbit(uint64_t n) { return __builtin_clzll(n); }

inline uint8_t lbit_from_right(uint64_t n) { return __builtin_ctzll(n); }

inline BitBoard pop_extract(BitBoard& b) {
    BitBoard bit = b & -b;
    b ^= bit;
    return bit;
}

inline Square pop(uint64_t& b) {
    Square idx = lbit(b);
    b &= b - 1;
    return idx;
}

static std::string print_binary(uint64_t n) {
    std::string result = "";
    do {
        if(n % 2 == 0) {
            result += '0';
        } else {
            result += '1';
        }
        n /= 2;
    } while (n > 0);
    return result;
}

static void print_bitboard(uint64_t bitboard) {
    std::cout << "   a b c d e f g h\n";
    std::cout << " +-----------------+\n";
    for (int row = 7; row >= 0; row--) {
        std::cout << row + 1 << "| ";
        for (int col = 7; col >= 0; col--) {
            int square = row * 8 + col;
            char piece = get_bit_64(bitboard, square) ? '1' : '.';
            std::cout << piece << ' ';
        }
        std::cout << "|" << row + 1 << "\n";
    }
    std::cout << " +-----------------+\n";
    std::cout << "   a b c d e f g h" << std::endl;
}

static uint64_t random_u64() {
    static std::mt19937_64 rng{ std::random_device{}() };
    return rng() & rng() & rng();
}

template<PieceType piece>
inline constexpr SliderType get_slider_type() {
    if constexpr (piece == PieceType::bishop || piece == PieceType::queen_diagonal) {
        return SliderType::diagonal;
    } else {
        return SliderType::orthogonal;
    }
}

template <Player player>
inline constexpr Player opponent() {
	return (player == Player::white) ? Player::black : Player::white;
}

inline std::string make_chess_notation(const Square square){
	// Convert column index (0-7) to letter (a-h)
	char column = 'a' + (square % 8);
	// Convert row index (0-7) to number (1-8)
	char row = '8' - (square / 8);
	return std::string(1, column) + std::string(1, row);
}

inline uint8_t notation_to_square(std::string notation) {
	int column = notation[0] - 'a';
	int row = 7 - (notation[1] - '1');
	return ((row * 8) + column);
}

inline void print_movecount(BitBoard start_square, BitBoard end_square, uint64_t cnt) {
	if (cnt) {
		std::cout << make_chess_notation(lbit(start_square)) << make_chess_notation(lbit(end_square)) 
        << ": " << std::to_string(cnt) << '\n';
    }
}