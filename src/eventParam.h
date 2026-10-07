#pragma once

#include <cstdint>
#include <string>

class EventParam {
    public:
    EventParam(std::string name, size_t size, int displayWidth) : name(name), size(size), displayWidth(displayWidth) {}
    std::string name;
    size_t size;
    int displayWidth;
};