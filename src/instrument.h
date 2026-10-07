#pragma once

#include "timeSeriesSource.h"
#include <string>
#include <vector>


class NamedInstrumentParamTemplate {
    public:
    char name[32];
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
};