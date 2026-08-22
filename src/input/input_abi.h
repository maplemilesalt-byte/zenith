#pragma once
#include <cstdint>

namespace ZenithInputABI {
constexpr std::uint64_t Address = 0xF000;
constexpr std::uint32_t Magic = 0x314E495A; // "ZIN1"
constexpr std::uint32_t Version = 1;

enum Button : std::uint32_t {
    A = 1u << 0,
    B = 1u << 1,
    X = 1u << 2,
    Y = 1u << 3,
    LB = 1u << 4,
    RB = 1u << 5,
    LeftStickClick = 1u << 6,
    RightStickClick = 1u << 7,
    View = 1u << 8,
    Menu = 1u << 9,
};

enum Dpad : std::uint32_t {
    Up = 1u << 0,
    Down = 1u << 1,
    Left = 1u << 2,
    Right = 1u << 3,
};

// Guest-visible layout. All fields are little-endian and naturally aligned.
struct State {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t buttons;
    std::uint32_t dpad;
    float leftX;
    float leftY;
    float rightX;
    float rightY;
    float leftTrigger;
    float rightTrigger;
};
static_assert(sizeof(State) == 40, "Zenith input ABI layout changed");
static_assert(offsetof(State, leftX) == 0x10, "Zenith input ABI offset changed");
static_assert(offsetof(State, rightX) == 0x18, "Zenith input ABI offset changed");
}
