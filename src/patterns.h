#pragma once

#include <vector>
#include <array>
#include <iostream>

#define MAX_PATTERN_LENGTH 256

using namespace std;

class PatternLibrary {
    public:
    //Top level vector per param
    //Next level vector per pattern
    //Array contains pattern values for the parent param
    PatternLibrary();
    vector<vector<array<int, MAX_PATTERN_LENGTH>>> paramPatterns;
    vector<int> getEvent(int patternIdx, int eventIdx);
    void writeEvent(vector<int> params, int patternIdx, int row);
    void addPattern();
    void addParam();
    void clear(int paramCount);
    vector<int> patternSequence;
};

ostream& operator<<(ostream& os, const PatternLibrary& patlib);
istream& operator>>(istream& is, PatternLibrary& patlib);
