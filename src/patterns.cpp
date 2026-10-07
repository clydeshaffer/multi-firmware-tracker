#include "patterns.h"

PatternLibrary::PatternLibrary() {
    addParam();
    addParam();
    addPattern();
    patternSequence.emplace_back(0);
}

vector<int> PatternLibrary::getEvent(int patternIdx, int eventIdx) {
    vector<int> event;
    for(auto& param: paramPatterns) {
        event.emplace_back(param[patternIdx][eventIdx]);
    }
    return event;
}

void PatternLibrary::writeEvent(vector<int> params, int patternIdx, int row) {
    int paramCount = params.size();
    if(paramCount == paramPatterns.size()) {
        for(int i = 0; i < paramCount; ++i) {
            paramPatterns[i][patternIdx][row] = params[i];
        }
    }
}

void PatternLibrary::addPattern() {
    for(auto& param: paramPatterns) {
        param.emplace_back();
    }
    paramPatterns.front().back().fill(255);
}

void PatternLibrary::addParam() {
    paramPatterns.emplace_back();
    if(paramPatterns.size() != 0) {
        paramPatterns.back().resize(paramPatterns.front().size());
    }
}