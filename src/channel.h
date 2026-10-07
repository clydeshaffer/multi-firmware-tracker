#pragma once

#include "instrument.h"
#include <vector>
#include <string>
#include "processStep.h"
#include "memWrite.h"
#include "eventParam.h"

class ChannelTemplate {
    public:
    TemplateInstrument templateInstrument;
    std::vector<Instrument> instruments;
    int stride = 4;
    std::vector<EventParam> eventParams;
    std::vector<NamedOffset> outputs;
    std::vector<ProcessStep*> noteHitSteps;
    std::vector<ProcessStep*> noteSustainSteps;

    void renderConfigUI();
};

class Channel {
    public:
    Channel(ChannelTemplate &b) : base(b) {}
    ChannelTemplate& base;
    int lastInstrumentIndex = -1;  
    std::vector<int> lastEventParams;  
    int framesSinceNote;
    std::vector<MemWrite> processNoteHit(int channelIdx, std::vector<int> params,int instrumentIdx);
    std::vector<MemWrite> processNoteTick(int channelIdx);
};