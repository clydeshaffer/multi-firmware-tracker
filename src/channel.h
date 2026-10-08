#pragma once

#include "instrument.h"
#include <vector>
#include <string>
#include "processStep.h"
#include "memWrite.h"
#include "eventParam.h"
#include "patterns.h"
#include <iostream>

class ChannelTemplate {
    public:
    int stride = 4;
    TemplateInstrument templateInstrument;
    std::vector<Instrument> instruments;
    std::vector<EventParam> eventParams;
    std::vector<NamedOffset> outputs;
    std::vector<ProcessStep*> noteHitSteps;
    std::vector<ProcessStep*> noteTickSteps;

    void renderConfigUI();
    ostream& exportInstruments(ostream& os);

    friend ostream& operator<<(ostream& os, const ChannelTemplate& ct);
    friend istream& operator>>(istream& is, ChannelTemplate& ct);
};

class Channel {
    public:
    Channel(ChannelTemplate &b) : base(b) {}
    ChannelTemplate& base;
    int lastInstrumentIndex = -1;  
    std::vector<int> lastEventParams;  
    int framesSinceNote;
    PatternLibrary patterns;
    std::vector<MemWrite> processNoteHit(int channelIdx, std::vector<int> params,int instrumentIdx);
    std::vector<MemWrite> processNoteTick(int channelIdx);
};