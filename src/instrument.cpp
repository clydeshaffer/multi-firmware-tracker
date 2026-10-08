#include "instrument.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "hash.h"
#include "serializationUtil.h"

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
        ImGui::InputText("Name", &param.name);
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
            ImGui::TextUnformatted(param.parent->name.c_str());
            ImGui::SameLine();
            ImGui::InputInt("Value", &param.value, 1, 1);
            ImGui::PopID();
        }
}

ostream& operator<<(ostream& os, const TemplateInstrument& ti) {
    os << "name \"" << ti.name << "\"" << std::endl;
    for(auto& envelope : ti.envelopes) {
        os << "envelope " << (*envelope) << "\n";
    }
    for(auto& param : ti.params) {
        os << "param \"" << param.name << "\" " << param.min << " " << param.max << std::endl;
    }
    os << "endStruct" << std::endl;
    return os;
}

istream& operator>>(istream& is, TemplateInstrument& ti) {
    std::string propName;
    is >> propName;
    while(propName != "endStruct") {
        switch(constHash(propName.c_str())) {
            case constHash("name"):
            getQuotedString(is, ti.name);
            break;
            case constHash("envelope"):
            ti.envelopes.emplace_back(deserializeTimeSeriesSourceSpec(is));
            break;
            case constHash("param"):
            ti.params.emplace_back();
            getQuotedString(is, ti.params.back().name);
            is >> ti.params.back().min >> ti.params.back().max;
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