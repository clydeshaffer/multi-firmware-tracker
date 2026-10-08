#include "memWrite.h"
#include "serializationUtil.h"

std::ostream& operator<<(std::ostream& os, const NamedOffset& no) {
    return os << "\"" << no.name << "\" " << no.offset << std::endl;
}

std::istream& operator>>(std::istream& is, NamedOffset& no) {
    return getQuotedString(is, no.name) >> no.offset;
}