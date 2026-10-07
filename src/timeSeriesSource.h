#pragma once

#include <cstdint>
#include <vector>

enum TimeSeriesSourceType {
    ARRAY
};

class TimeSeriesSource {
    public:
    virtual int evaluate(int initial, int t) = 0;
    virtual void draw_instrument_ui() = 0;
    virtual TimeSeriesSourceType type() = 0;
};

class TimeSeriesSourceSpec {
    public:
    virtual void draw_setup_ui() = 0;
    virtual TimeSeriesSource* create() = 0;
    char name[64];
};

class ArrayEnvSourceSpec : public TimeSeriesSourceSpec {
    public:
        void draw_setup_ui();
        TimeSeriesSource* create();
    public:
        int min, max;
};

class ArrayEnvSource : public TimeSeriesSource {
    public:
        int evaluate(int initial, int t);
        void draw_instrument_ui();
        ArrayEnvSource(ArrayEnvSourceSpec* spec);
        TimeSeriesSourceType type();
    private:
        ArrayEnvSourceSpec* parent;
        std::vector<int> envelopeData;
};