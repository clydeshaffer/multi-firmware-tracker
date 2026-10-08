#pragma once

#include <cstdint>
#include <string>
#include <iostream>

class EventParam {
    public:
    EventParam() = default;
    EventParam(std::string name, size_t size, int displayWidth) : name(name), size(size), displayWidth(displayWidth) {}
    std::string name;
    size_t size;
    int displayWidth;
};

std::ostream& operator<<(std::ostream& os, const EventParam& ep);
std::istream& operator>>(std::istream& is, EventParam& ep);