#include "eventParam.h"
#include "serializationUtil.h"

std::ostream& operator<<(std::ostream& os, const EventParam& ep) {
    return os << "\"" << ep.name << "\" " << ep.size << " " << ep.displayWidth << std::endl;
}

std::istream& operator>>(std::istream& is, EventParam& ep) {
    return getQuotedString(is, ep.name) >> ep.size >> ep.displayWidth;
}
