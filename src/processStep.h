#pragma once

#include <cstdint>
#include <vector>
#include "memWrite.h"
#include "instrument.h"
#include "eventParam.h"

class ProcessingState {
    public:
        ProcessingState(int t, std::vector<int>& params,Instrument& instrument) : t(t), params(params), instrument(instrument), acc(0) {}
        int t;
        unsigned int acc;
        std::vector<int>& params;
        std::vector<ParamWrite> output;
        Instrument& instrument;
};

class ProcessStepDrawContext {
    public:
    ProcessStepDrawContext(TemplateInstrument& ti, std::vector<NamedOffset>& o, std::vector<EventParam>& ep) : templateInstrument(ti), outputs(o), eventParams(ep) {}
    TemplateInstrument& templateInstrument;
    std::vector<NamedOffset>& outputs;
    std::vector<EventParam>& eventParams;
};

class ProcessStep {
    public:
    virtual void exec(ProcessingState* st) = 0;
    virtual void draw_ui(ProcessStepDrawContext& ctx) = 0;
};

class ProcessGetN : public ProcessStep {
    public:
    ProcessGetN(int n) : n(n) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int n;
};

class ProcessGetT : public ProcessStep {
    public:
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
};

class ProcessGetParam : public ProcessStep {
    public:
    ProcessGetParam(int paramIndex) : paramIndex(paramIndex) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int paramIndex;
};

class ProcessAddN : public ProcessStep {
    public:
    ProcessAddN(int increment) : increment(increment) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int increment;
};

class ProcessRemap: public ProcessStep {
    public:
    ProcessRemap(int a, int b, int x, int y) : a(a), b(b), x(x), y(y) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int a,b,x,y;
};

class ProcessClamp : public ProcessStep {
    public:
    ProcessClamp(int lower, int upper) : lower(lower), upper(upper) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    int lower, upper;
};

class ProcessFetchEnvelope : public ProcessStep {
    public:
    ProcessFetchEnvelope(int envIndex) : envIndex(envIndex) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int envIndex;
};

class ProcessFetchInstrumentParam : public ProcessStep {
    public:
    ProcessFetchInstrumentParam(int paramIndex) : paramIndex(paramIndex) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int paramIndex;
};

class ProcessAddInstrumentParam : public ProcessStep {
    public:
    ProcessAddInstrumentParam(int paramIndex) : paramIndex(paramIndex) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int paramIndex;
};

class ProcessSendMemWrite : public ProcessStep {
    public:
    ProcessSendMemWrite(int paramIndex, bool highByte) :  paramIndex(paramIndex), highByte(highByte) {}
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
    private:
    int paramIndex;
    bool highByte;
};

class ProcessLookupPitch : public ProcessStep {
    public:
    void exec(ProcessingState* st);
    void draw_ui(ProcessStepDrawContext& ctx);
};