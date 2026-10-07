#pragma once

#include <cstdint>
#include <string>

class MemWrite {
    public:
        uint16_t address;
        uint8_t value;
};

class ParamWrite {
    public:
        uint8_t paramIdx;
        uint8_t value;
};

class NamedOffset {
    public:
    NamedOffset(std::string name, int offset) : name(name), offset(offset) {}
    std::string name;
    int offset;
};