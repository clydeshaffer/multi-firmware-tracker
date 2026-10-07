#include "channel.h"
#include "imgui.h"

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
        for(auto& step : noteSustainSteps) {
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

    ProcessingState ps(framesSinceNote, lastEventParams, base.instruments[lastInstrumentIndex]);
    for(auto& step : base.noteSustainSteps) {
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