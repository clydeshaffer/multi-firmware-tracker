#include "channel.h"
#include "imgui.h"
#include "hash.h"
#include "serializationUtil.h"

void ChannelTemplate::renderConfigUI() {
    int i = 0;
    if(ImGui::BeginChild("General Properties",  ImVec2(0,0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
        ImGui::InputInt("Stride", &stride);
        templateInstrument.renderConfigUI();
    }
    ImGui::EndChild();

    if(ImGui::BeginChild("Event Params",  ImVec2(0,0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
        ImGui::TextUnformatted("Event Params");
        ImGui::Separator();
        i = 0;
        for(auto& param : eventParams) {
            ImGui::PushID(i++);
            ImGui::TextUnformatted(param.name.c_str());
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    if(ImGui::BeginChild("Memory Outputs",  ImVec2(0,0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
        ImGui::TextUnformatted("Memory Outputs");
        ImGui::Separator();
        i = 0;
        for(auto& output : outputs) {
            ImGui::PushID(i++);
            ImGui::TextUnformatted(output.name.c_str());
            ImGui::SameLine();
            ImGui::Text("%x", output.offset);
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    ProcessStepDrawContext pctx(templateInstrument, outputs, eventParams);
    if(ImGui::BeginChild("Note Hit Processing",  ImVec2(0,0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
        ImGui::TextUnformatted("Note Hit Processing");
        ImGui::Separator();
        i = 0;
        for(auto& step : noteHitSteps) {
            ImGui::PushID(i++);
            step->draw_ui(pctx);
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    if(ImGui::BeginChild("Note Tick Processing",  ImVec2(0,0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
        ImGui::TextUnformatted("Note Tick Processing");
        ImGui::Separator();
        i = 0;
        for(auto& step : noteTickSteps) {
            ImGui::PushID(i++);
            step->draw_ui(pctx);
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

}

std::vector<MemWrite> Channel::processNoteHit(int channelIdx, std::vector<int> params, int instrumentIdx) {
    if((instrumentIdx < 0) || (instrumentIdx >= base.instruments.size())) {
        std::vector<MemWrite> out;
        return out;
    }
    framesSinceNote = 0;
    lastEventParams = params;
    lastInstrumentIndex = instrumentIdx;
    ProcessingState ps(0, params, base.instruments[lastInstrumentIndex]);
    for(auto& step : base.noteHitSteps) {
        step->exec(&ps);
    }
    
    std::vector<MemWrite> out;
    for(auto& pw : ps.output) {
        MemWrite mw;
        mw.address = base.outputs[pw.paramIdx].offset + (channelIdx * base.stride);
        mw.value = pw.value;
        out.emplace_back(mw);
    }
    return out;
}

std::vector<MemWrite> Channel::processNoteTick(int channelIdx) {
    if((lastInstrumentIndex < 0) || (lastInstrumentIndex >= base.instruments.size())) {
        std::vector<MemWrite> out;
        return out;
    }

    if(base.instruments[lastInstrumentIndex].vacant) {
        return silence(channelIdx);
    }

    ProcessingState ps(framesSinceNote, lastEventParams, base.instruments[lastInstrumentIndex]);
    for(auto& step : base.noteTickSteps) {
        step->exec(&ps);
    }
    
    std::vector<MemWrite> out;
    for(auto& pw : ps.output) {
        MemWrite mw;
        mw.address = base.outputs[pw.paramIdx].offset + (channelIdx * base.stride);
        mw.value = pw.value;
        out.emplace_back(mw);
    }
    ++framesSinceNote;
    return out;
}

std::vector<MemWrite> Channel::silence(int channelIdx) {
    if((lastInstrumentIndex < 0) || (lastInstrumentIndex >= base.instruments.size())) {
        std::vector<MemWrite> out;
        return out;
    }

    ProcessingState ps(framesSinceNote, lastEventParams, base.instruments[lastInstrumentIndex]);
    for(auto& step : base.silenceChannelSteps) {
        step->exec(&ps);
    }
    
    std::vector<MemWrite> out;
    for(auto& pw : ps.output) {
        MemWrite mw;
        mw.address = base.outputs[pw.paramIdx].offset + (channelIdx * base.stride);
        mw.value = pw.value;
        out.emplace_back(mw);
    }
    lastInstrumentIndex = -1;
    return out;
}

void ChannelTemplate::writeBinary(ostream& os) {
	os.put((char) instruments.size());

    std::streampos instPointerTablePos = os.tellp();
    std::vector<std::streampos> instPositions;

    for(auto& instr : instruments) {
        os.put(0); os.put(0);
	}

	for(auto& instr : instruments) {
        instPositions.emplace_back(os.tellp());
		instr.writeBinary(os);
	}

    std::streampos currentEnd = os.tellp();
    os.seekp(instPointerTablePos);
    for(auto& instPos : instPositions) {
        uint16_t offset = instPos;
        os.put((char) (offset & 0xFF));
        os.put((char) (offset >> 8));
	}
    os.seekp(currentEnd);
}

void ChannelTemplate::addInstrument(Instrument instrument) {
    for(int instIdx = 0; instIdx < instruments.size(); ++instIdx) {
        if(instruments[instIdx].vacant) {
            instruments[instIdx] = std::move(instrument);
            return;
        }
    }
    instruments.emplace_back(std::move(instrument));
}

bool ChannelTemplate::instrumentExists(int idx) {
    if(idx >= instruments.size()) return false;
    return !(instruments[idx].vacant);
}

ostream& ChannelTemplate::exportInstruments(ostream& os) {
    for(auto& instr : instruments) {
        os << "instrument" << std::endl;
        os << instr;
    }
    return os;
}

void ChannelTemplate::clearConfig() {
    for(auto& noteHitStep : noteHitSteps) {
        delete noteHitStep;
    }
    for(auto& noteTickStep : noteTickSteps) {
        delete noteTickStep;
    }
}

//Serializer
ostream& operator<<(ostream& os, const ChannelTemplate& ct) {
    os << "stride " << ct.stride << std::endl;
    os << "templateInstrument " << ct.templateInstrument << std::endl;
    for(auto& eventParam : ct.eventParams) {
        os << "eventParam " << eventParam << std::endl;
    }
    for(auto& output : ct.outputs) {
        os << "output " << output << std::endl;
    }
    for(auto& noteHitStep : ct.noteHitSteps) {
        os << "noteHitStep " << (*noteHitStep) << std::endl;
    }
    for(auto& noteTickStep : ct.noteTickSteps) {
        os << "noteTickStep " << (*noteTickStep) << std::endl;
    }
    for(auto& silenceChannelStep : ct.silenceChannelSteps) {
        os << "silenceChannelStep " << (*silenceChannelStep) << std::endl;
    }
    os << "endStruct" << std::endl;
    return os;
}

//Deserializer
istream& operator>>(istream& is, ChannelTemplate& ct) {
    std::string propName;
    is >> propName;
    while(propName != "endStruct") {
        switch(constHash(propName.c_str())) {
            //Used in firmware configs
            case constHash("stride"):
            is >> ct.stride;
            break;
            case constHash("templateInstrument"):
            is >> ct.templateInstrument;
            break;
            case constHash("eventParam"):
            ct.eventParams.emplace_back();
            if(!(is >> ct.eventParams.back())) {
                ct.eventParams.pop_back();
            }
            break;
            case constHash("output"):
            ct.outputs.emplace_back();
            if(!(is >> ct.outputs.back())) {
                ct.outputs.pop_back();
            }
            break;
            case constHash("noteHitStep"):
            ct.noteHitSteps.emplace_back(deserializeProcessStep(is));
            break;
            case constHash("noteTickStep"):
            ct.noteTickSteps.emplace_back(deserializeProcessStep(is));
            break;
            case constHash("silenceChannelStep"):
            ct.silenceChannelSteps.emplace_back(deserializeProcessStep(is));
            break;

            //Used in song loading
            case constHash("instrument"):
            ct.instruments.emplace_back(ct.templateInstrument.create());
            Instrument& inst = ct.instruments.back();
            is >> inst;
            break;
        }
        if(is.fail()) {
            cout << "stream marked failed after handling property: " << propName << std::endl;
            return is;
        }
        is >> propName;
    }
    return is;
}
