#pragma once

#include "timeSeriesSource.h"
#include <string>
#include <vector>
#include <iostream>

using namespace std;

class NamedInstrumentParamTemplate {
    public:
    std::string name;
    int min;
    int max;
};

class NamedInstrumentParam {
    public:
    NamedInstrumentParamTemplate* parent;
    int value;
};

class Instrument {
    public:
    std::string name;
    std::vector<TimeSeriesSource*> envelopes;
    std::vector<NamedInstrumentParam> params;
    void renderConfigUI();
};

class TemplateInstrument {
    public:
    std::string name;
    std::vector<TimeSeriesSourceSpec*> envelopes;
    std::vector<NamedInstrumentParamTemplate> params;
    void renderConfigUI();
    Instrument create();

    friend ostream& operator<<(ostream& os, const TemplateInstrument& ti);
    friend istream& operator>>(istream& is, TemplateInstrument& ti);
};

ostream& operator<<(ostream& os, const TemplateInstrument& ti);
istream& operator>>(istream& is, TemplateInstrument& ti);