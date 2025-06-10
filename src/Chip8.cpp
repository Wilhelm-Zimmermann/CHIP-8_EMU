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
    opcode = 0;
    I = 0;
    sp = 0;

    memset(memory, 0, sizeof(memory));
    memset(V, 0, sizeof(V));
    memset(video, 0, sizeof(video));
    memset(stack, 0, sizeof(stack));

    for (int i = 0; i < 80; ++i)
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

void Chip8::UpdateTimers()
{
    if (delayTimer > 0)
    {
        --delayTimer;
    }
    if (soundTimer > 0)
    {
        if (--soundTimer == 0)
        {
            std::cout << "BEEP!" << std::endl;
        }
    }
}

void Chip8::Cycle()
{

    opcode = (memory[pc] << 8) | memory[pc + 1];

#if DEBUG
    std::cout << "PC: 0x" << std::hex << pc << "  Opcode: 0x" << opcode << std::endl;
#endif

    pc += 2;

    switch (opcode & 0xF000)
    {
    case 0x0000:
        switch (opcode & 0x00FF)
        {
        case 0x00E0:
            memset(video, 0, sizeof(video));
            break;
        case 0x00EE:
            if (sp > 0)
            {
                --sp;
                pc = stack[sp];
            }
            break;
        default:
            goto UNKNOWN_OPCODE;
        }
        break;

    case 0x1000:
        pc = opcode & 0x0FFF;
        break;

    case 0x2000:
        if (sp < 15)
        {
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
        }
        break;

    case 0x3000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        if (V[Vx_idx] == byte)
        {
            pc += 2;
        }
        break;
    }

    case 0x4000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        if (V[Vx_idx] != byte)
        {
            pc += 2;
        }
        break;
    }

    case 0x5000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t Vy_idx = (opcode & 0x00F0) >> 4;
        if (V[Vx_idx] == V[Vy_idx])
        {
            pc += 2;
        }
        break;
    }

    case 0x6000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        V[Vx_idx] = byte;
        break;
    }

    case 0x7000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        V[Vx_idx] += byte;
        break;
    }

    case 0x8000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t Vy_idx = (opcode & 0x00F0) >> 4;
        switch (opcode & 0x000F)
        {
        case 0x0000:
            V[Vx_idx] = V[Vy_idx];
            break;
        case 0x0001:
            V[Vx_idx] |= V[Vy_idx];
            break;
        case 0x0002:
            V[Vx_idx] &= V[Vy_idx];
            break;
        case 0x0003:
            V[Vx_idx] ^= V[Vy_idx];
            break;
        case 0x0004:
        {
            uint16_t sum = V[Vx_idx] + V[Vy_idx];
            V[0xF] = (sum > 255);
            V[Vx_idx] = sum & 0xFF;
            break;
        }
        case 0x0005:
            V[0xF] = (V[Vx_idx] > V[Vy_idx]);
            V[Vx_idx] -= V[Vy_idx];
            break;
        case 0x0006:
            V[0xF] = V[Vx_idx] & 0x1;
            V[Vx_idx] >>= 1;
            break;
        case 0x0007:
            V[0xF] = (V[Vy_idx] > V[Vx_idx]);
            V[Vx_idx] = V[Vy_idx] - V[Vx_idx];
            break;
        case 0x000E:
            V[0xF] = V[Vx_idx] >> 7;
            V[Vx_idx] <<= 1;
            break;
        default:
            goto UNKNOWN_OPCODE;
        }
        break;
    }

    case 0x9000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t Vy_idx = (opcode & 0x00F0) >> 4;
        if (V[Vx_idx] != V[Vy_idx])
        {
            pc += 2;
        }
        break;
    }

    case 0xA000:
        I = opcode & 0x0FFF;
        break;

    case 0xB000:
        pc = (opcode & 0x0FFF) + V[0];
        break;

    case 0xC000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t byte = opcode & 0x00FF;
        V[Vx_idx] = (rand() % 256) & byte;
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
                    if (yPos + row < 32 && xPos + col < 64)
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

    case 0xE000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        uint8_t key = V[Vx_idx];
        switch (opcode & 0x00FF)
        {
        case 0x009E:
            if (keypad[key])
            {
                pc += 2;
            }
            break;
        case 0x00A1:
            if (!keypad[key])
            {
                pc += 2;
            }
            break;
        default:
            goto UNKNOWN_OPCODE;
        }
        break;
    }

    case 0xF000:
    {
        uint8_t Vx_idx = (opcode & 0x0F00) >> 8;
        switch (opcode & 0x00FF)
        {
        case 0x0007:
            V[Vx_idx] = delayTimer;
            break;
        case 0x000A:
        {
            bool key_pressed = false;
            for (int i = 0; i < 16; ++i)
            {
                if (keypad[i])
                {
                    V[Vx_idx] = i;
                    key_pressed = true;
                    break;
                }
            }

            if (!key_pressed)
            {
                pc -= 2;
            }
            break;
        }
        case 0x0015:
            delayTimer = V[Vx_idx];
            break;
        case 0x0018:
            soundTimer = V[Vx_idx];
            break;
        case 0x001E:
            I += V[Vx_idx];
            break;
        case 0x0029:
            I = 0x50 + (V[Vx_idx] * 5);
            break;
        case 0x0033:
            memory[I] = V[Vx_idx] / 100;
            memory[I + 1] = (V[Vx_idx] / 10) % 10;
            memory[I + 2] = V[Vx_idx] % 10;
            break;
        case 0x0055:
            for (int i = 0; i <= Vx_idx; ++i)
            {
                memory[I + i] = V[i];
            }
            break;
        case 0x0065:
            for (int i = 0; i <= Vx_idx; ++i)
            {
                V[i] = memory[I + i];
            }
            break;
        default:
            goto UNKNOWN_OPCODE;
        }
        break;
    }

    default:
    UNKNOWN_OPCODE:
        std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
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