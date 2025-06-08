#pragma once
#include <cstdint>
#include <string>

extern uint8_t fontset[80];

class Chip8
{
public:
    Chip8();
    void LoadROM(const std::string &filename);
    void Cycle();

    uint32_t video[64 * 32]{};
    uint8_t keypad[16]{};

private:
    uint8_t memory[4096]{};
    uint8_t V[16]{};
    uint16_t I{};
    uint16_t pc{};

    uint16_t stack[16]{};
    uint8_t sp{};

    uint8_t delayTimer{};
    uint8_t soundTimer{};

    uint16_t opcode;
};