#include "serializationUtil.h"

std::istream& getQuotedString(std::istream& is, std::string& str) {
    char quote;
    is >> quote;
    if(quote == '"') {
        std::getline(is, str, '"');
        if(is.eof()) {
            std::cout << "got EOF before the next quote???" << std::endl;
        }
        if(is.fail()) {
            std::cout << "fail bit after calling getline" << std::endl;
        }
        return is;
    } else {
        std::cout << "expected quote mark and got " << quote << std::endl;
        is.setstate(std::ios_base::failbit);
        return is;
    }
}