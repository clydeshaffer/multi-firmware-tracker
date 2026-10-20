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
    std::vector<unique_ptr<TimeSeriesSource>> envelopes;
    std::vector<NamedInstrumentParam> params;
    bool vacant = false;

    Instrument() = default;
    Instrument(const Instrument& other);
    Instrument& operator=(const Instrument& other);

    Instrument(Instrument&& other) noexcept;
    Instrument& operator=(Instrument&& other) noexcept;
    ~Instrument();
    
    void renderConfigUI();
    void writeBinary(ostream& os);
    friend ostream& operator<<(ostream& os, const Instrument& inst);
    friend istream& operator>>(istream& is, Instrument& inst);
};

class TemplateInstrument {
    public:
    std::string name;
    std::vector<unique_ptr<TimeSeriesSourceSpec>> envelopes;
    std::vector<NamedInstrumentParamTemplate> params;
    void renderConfigUI();
    Instrument create();

    friend ostream& operator<<(ostream& os, const TemplateInstrument& ti);
    friend istream& operator>>(istream& is, TemplateInstrument& ti);
};

ostream& operator<<(ostream& os, const TemplateInstrument& ti);
istream& operator>>(istream& is, TemplateInstrument& ti);

ostream& operator<<(ostream& os, const Instrument& inst);
istream& operator>>(istream& is, Instrument& inst);
