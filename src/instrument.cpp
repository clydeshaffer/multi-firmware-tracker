#include "instrument.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

Instrument TemplateInstrument::create() {
    Instrument instr;
    instr.name = name;
    for(auto& envelope : envelopes) {
        instr.envelopes.emplace_back(envelope->create());
    }
    for(auto& param : params) {
        NamedInstrumentParam nip;
        nip.parent = &param;
        nip.value = 0;
        instr.params.emplace_back(nip);
    }
    return instr;
}

void TemplateInstrument::renderConfigUI() {
    int i = 0;
    for(auto& envelope : envelopes) {
        ImGui::PushID(i++);
        envelope->draw_setup_ui();    
        ImGui::PopID();
    }
    for(auto& param : params) {
        ImGui::PushID(i++);
        ImGui::InputText("Name", param.name, 32);
        ImGui::SameLine();
        ImGui::InputInt("Min", &param.min);
        ImGui::SameLine();
        ImGui::InputInt("Max", &param.max);
        ImGui::PopID();
    }
}

void Instrument::renderConfigUI() {
        int i = 0;
        ImGui::InputText("Name", &name);
        for(auto& envelope : envelopes) {
            ImGui::PushID(i++);
            envelope->draw_instrument_ui();    
            ImGui::PopID();
        }
        for(auto& param : params) {
            ImGui::PushID(i++);
            ImGui::TextUnformatted(param.parent->name);
            ImGui::SameLine();
            ImGui::InputInt("Value", &param.value, 1, 1);
            ImGui::PopID();
        }
}