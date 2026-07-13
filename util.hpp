#include <cstdio>
#include <iostream>
#include <random>
#include <string>

#include "defaults.hpp"
#include "move.hpp"

#pragma once

inline bool get_bit_64(uint64_t num, uint8_t pos) { return (num & (1ULL << (pos))) != 0; }

inline constexpr uint64_t square_to_mask(Square s) { return 1ULL << s; }

inline uint8_t lb(uint64_t n) { return __builtin_clzll(n); }

inline uint8_t lbr(uint64_t n) {
    if (n == 0) { // TODO: remove check.
        throw std::invalid_argument("lbr called with zero");
    }    
    return __builtin_ctzll(n); 
}

inline BitBoard pop_extract(BitBoard &b)
{
    BitBoard bit = b & -b;
    b ^= bit;
    return bit;
}

inline Square pop(uint64_t &b)
{
    Square idx = lbr(b);
    b &= b - 1;
    return idx;
}

// Splits pieces into a pinned subset (those on a pin line) and mutates
// the input to hold the remaining, unpinned subset.
inline BitBoard extract_pinned(BitBoard &pieces, BitBoard nopin)
{
    BitBoard pinned = pieces & ~nopin;
    pieces ^= pinned;
    return pinned;
}

static std::string print_binary(uint64_t n)
{
    std::string result = "";
    do
    {
        if (n % 2 == 0)
        {
            result += '0';
        }
        else
        {
            result += '1';
        }
        n /= 2;
    } while (n > 0);
    return result;
}

static void print_bitboard(uint64_t bitboard, const std::string &label = "")
{
    if (!label.empty())
    {
        std::cout << label << ":\n";
    }
    std::cout << "   a b c d e f g h\n";
    std::cout << " +-----------------+\n";
    for (int row = 7; row >= 0; row--)
    {
        std::cout << row + 1 << "| ";
        for (int col = 7; col >= 0; col--)
        {
            int square = row * 8 + col;
            char piece = get_bit_64(bitboard, square) ? '1' : '.';
            std::cout << piece << ' ';
        }
        std::cout << "|" << row + 1 << "\n";
    }
    std::cout << " +-----------------+\n";
    std::cout << "   a b c d e f g h" << std::endl;
}

static uint64_t random_u64()
{
    static std::mt19937_64 rng{std::random_device{}()};
    return rng() & rng() & rng();
}

template <Player player>
inline constexpr Player opponent()
{
    return (player == Player::white) ? Player::black : Player::white;
}

// TODO: make this a lookup table.
inline constexpr uint8_t notation_to_square(const char *notation)
{
    int column = 'h' - notation[0];
    int row = notation[1] - '1';
    return ((row * 8) + column);
}

template <bool print_debug_info = false>
inline void print_movecount(Move move, size_t cnt)
{
    std::cout << MoveEncoding::move_to_string(move) << ": " << std::to_string(cnt) << '\n';

    if constexpr (print_debug_info)
    {
        std::cout << "Move binary: " << print_binary(move) << '\n';
        std::cout << "From square: " << std::to_string(MoveEncoding::get_from_square(move)) << '\n';
        std::cout << "To square: " << std::to_string(MoveEncoding::get_to_square(move)) << '\n';
        print_bitboard(square_to_mask(MoveEncoding::get_from_square(move)) | square_to_mask(MoveEncoding::get_to_square(move)));
    }
}

// Helper function to format numbers with commas
inline std::string format_number(size_t number)
{
    std::string str = std::to_string(number);
    int pos = str.length() - 3;
    while (pos > 0)
    {
        str.insert(pos, ",");
        pos -= 3;
    }
    return str;
}

template <Player player, bool kingside>
constexpr inline BitBoard get_corner() {
    if constexpr (player == Player::white) {
        return kingside ? c_bottom_right_corner : c_bottom_left_corner;
    } else {
        return kingside ? c_top_right_corner : c_top_left_corner;
    }
}