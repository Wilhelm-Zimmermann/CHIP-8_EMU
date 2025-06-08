#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <random>
#include <cstring>

#define DEBUG true

uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80};

Chip8::Chip8()
{
    pc = 0x200;

    for (unsigned int i = 0; i < 80; i++)
    {
        memory[0x50 + i] = fontset[i];
    }

    srand(time(NULL));
}

void Chip8::LoadROM(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        std::streampos size = file.tellg();
        char *buffer = new char[size];

        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        for (long i = 0; i < size; i++)
        {
            memory[0x200 + i] = buffer[i];
        }

        delete[] buffer;
        std::cout << "ROM loaded successfully!" << std::endl;
    }
    else
    {
        std::cerr << "Failed to open ROM file: " << filename << std::endl;
    }
}

void Chip8::Cycle()
{
    opcode = (memory[pc] << 8) | memory[pc + 1];

    #ifdef DEBUG
    std::cout << "PC: 0x" << std::hex << pc << "  Opcode: 0x" << opcode << std::endl;
    #endif
    pc += 2;

    if (sp < 15)
    {
        stack[sp] = pc;
        ++sp;
        pc = opcode & 0x0FFF;
    }
    else
    {
        std::cerr << "Error: Stack overflow!" << std::endl;
        while (true)
            ;
    }

    if (sp > 0)
    {
        --sp;
        pc = stack[sp];
    }
    else
    {
        std::cerr << "Error: Stack underflow!" << std::endl;
        while (true)
            ;
    }

    switch (opcode & 0xF000)
    {
    case 0x0000:
        switch (opcode & 0x00FF)
        {
        case 0x00E0:
            memset(video, 0, sizeof(video));
            break;
        case 0x00EE:
            --sp;
            pc = stack[sp];
            break;
        default:
            break;
        }
        break;

    case 0x1000:
        pc = opcode & 0x0FFF;
        break;

    case 0x2000:
        stack[sp] = pc;
        ++sp;
        pc = opcode & 0x0FFF;
        break;

    case 0x3000:
    {
        uint8_t Vx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        if (V[Vx] == byte)
        {
            pc += 2;
        }
        break;
    }

    case 0x6000:
    {
        uint8_t Vx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        V[Vx] = byte;
        break;
    }

    case 0x7000:
    {
        uint8_t Vx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        V[Vx] += byte;
        break;
    }

    case 0x8000:
    {
        uint8_t Vx = (opcode & 0x0F00) >> 8;
        uint8_t Vy = (opcode & 0x00F0) >> 4;
        switch (opcode & 0x000F)
        {
        case 0x0000:
            V[Vx] = V[Vy];
            break;
        case 0x0002:
            V[Vx] &= V[Vy];
            break;
        case 0x0004:
        {
            uint16_t sum = V[Vx] + V[Vy];
            V[0xF] = (sum > 255);
            V[Vx] = sum & 0xFF;
            break;
        }

        default:
            goto UNKNOWN_OPCODE;
        }
        break;
    }

    case 0xA000:

        I = opcode & 0x0FFF;
        break;

    case 0xC000:
    {
        uint8_t Vx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        V[Vx] = (rand() % 256) & byte;
        break;
    }

    case 0xD000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t Vy_idx = (opcode & 0x00F0) >> 4;
        uint8_t height = opcode & 0x000F;

        uint8_t xPos = V[Vx_idx] % 64;
        uint8_t yPos = V[Vy_idx] % 32;

        V[0xF] = 0;

        for (unsigned int row = 0; row < height; ++row)
        {
            uint8_t sprite_byte = memory[I + row];

            for (unsigned int col = 0; col < 8; ++col)
            {

                if ((sprite_byte & (0x80 >> col)) != 0)
                {

                    if ((yPos + row < 32) && (xPos + col < 64))
                    {
                        int screen_idx = (yPos + row) * 64 + (xPos + col);

                        if (video[screen_idx] == 0xFFFFFFFF)
                        {
                            V[0xF] = 1;
                        }
                        video[screen_idx] ^= 0xFFFFFFFF;
                    }
                }
            }
        }
        break;
    }

    case 0xF000:
    {
        uint8_t Vx = (opcode & 0x0F00) >> 8;
        switch (opcode & 0x00FF)
        {
        case 0x0007:
            V[Vx] = delayTimer;
            break;
        case 0x0015:
            delayTimer = V[Vx];
            break;
        case 0x0029:
            I = 0x50 + (V[Vx] * 5);
            break;
        case 0x0033:
            memory[I] = V[Vx] / 100;
            memory[I + 1] = (V[Vx] / 10) % 10;
            memory[I + 2] = V[Vx] % 10;
            break;
        default:
            goto UNKNOWN_OPCODE;
        }
        break;
    }

    default:
    UNKNOWN_OPCODE:
        std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
    }

    if (delayTimer > 0)
    {
        --delayTimer;
    }
    if (soundTimer > 0)
    {
        --soundTimer;
        if (soundTimer == 0)
        {
            std::cout << "BEEP!" << std::endl;
        }
    }
}