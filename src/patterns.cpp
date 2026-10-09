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

void PatternLibrary::clear(int paramCount) {
	patternSequence.resize(1);
	patternSequence[0] = 0;
	paramPatterns.resize(paramCount);
	for(auto& paramTable : paramPatterns) {
		paramTable.resize(1);
		paramTable.front().fill(255);
	}
}

ostream& operator<<(ostream& os, const PatternLibrary& patlib) {
    os << patlib.paramPatterns.size() << std::endl;
    for(auto& patternsForParam : patlib.paramPatterns) {
        os << patternsForParam.size() << std::endl;
        for(auto& patParamArray : patternsForParam) {
            os << patParamArray.size() << std::endl;
            for(auto& val : patParamArray) {
                os << " " << val ;
            }        
            os << std::endl;
        }
    }
    os << patlib.patternSequence.size() << std::endl;
    for(auto& val : patlib.patternSequence) {
        os << " " << val;
    }
    os << std::endl;
    return os;
}

istream& operator>>(istream& is, PatternLibrary& patlib) {
    int paramCount, patternCount, arraySize, sequenceLength;
    is >> paramCount;
    patlib.paramPatterns.resize(paramCount);
    for(int paramIdx = 0; paramIdx < paramCount; ++paramIdx) {
        is >> patternCount;
        patlib.paramPatterns[paramIdx].resize(patternCount);
        for(int patternIdx = 0; patternIdx < patternCount; ++patternIdx) {
            is >> arraySize;
            //Array size is fixed at 256
            std::array<int, MAX_PATTERN_LENGTH>& currentArray = patlib.paramPatterns[paramIdx][patternIdx];
            for(int arrayIdx = 0; arrayIdx < arraySize; ++arrayIdx) {
                is >> currentArray[arrayIdx];
            }
        }
    }
    is >> sequenceLength;
    patlib.patternSequence.resize(sequenceLength);
    for(int seqId = 0; seqId < sequenceLength; ++seqId) {
        is >> patlib.patternSequence[seqId];
    }
    return is;
}
