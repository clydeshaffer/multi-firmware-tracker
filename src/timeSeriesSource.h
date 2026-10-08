#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include "serializationUtil.h"

using namespace std;

enum TimeSeriesSourceType {
    ARRAY
};

class TimeSeriesSource {
    public:
    virtual int evaluate(int initial, int t) = 0;
    virtual void draw_instrument_ui() = 0;
    virtual TimeSeriesSourceType type() = 0;
    virtual void print(std::ostream& os) const {}
    virtual void scan(std::istream& is) {}
};

class TimeSeriesSourceSpec {
    public:
    virtual void draw_setup_ui() = 0;
    virtual TimeSeriesSource* create() = 0;
    std::string name;
    virtual void print(std::ostream& os) const {}
    virtual void scan(std::istream& is) {}
};

class ArrayEnvSourceSpec : public TimeSeriesSourceSpec {
    public:
        void draw_setup_ui();
        TimeSeriesSource* create();
        void print(std::ostream& os) const override;
        void scan(std::istream& is) override;

        int min, max;
};

class ArrayEnvSource : public TimeSeriesSource {
    public:
        int evaluate(int initial, int t);
        void draw_instrument_ui();
        ArrayEnvSource(ArrayEnvSourceSpec* spec);
        TimeSeriesSourceType type();
        void print(std::ostream& os) const override;
        void scan(std::istream& is) override;
    private:
        ArrayEnvSourceSpec* parent;
        std::vector<int> envelopeData;
};

ostream& operator<<(ostream& os, const TimeSeriesSourceSpec& tss);

ostream& operator<<(ostream& os, const TimeSeriesSource& tss);

TimeSeriesSourceSpec* deserializeTimeSeriesSourceSpec(std::istream& is);

TimeSeriesSource* deserializeTimeSeriesSource(std::istream& is);