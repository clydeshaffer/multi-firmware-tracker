#include "processStep.h"
#include "imgui.h"
#include "hash.h"


#define PITCH_TABLE_LENGTH 216
static uint8_t pitch_table[PITCH_TABLE_LENGTH] = {
    0x00, 0x4D, 0x00, 0x51, 0x00, 0x56, 0x00, 0x5B, 0x00, 0x61, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x73, 0x00, 0x7A, 0x00, 0x81, 0x00, 0x89, 0x00, 0x91,
    0x00, 0x99, 0x00, 0xA2, 0x00, 0xAC, 0x00, 0xB6, 0x00, 0xC1, 0x00, 0xCD, 0x00, 0xD9, 0x00, 0xE6, 0x00, 0xF3, 0x01, 0x02, 0x01, 0x11, 0x01, 0x21,
    0x01, 0x33, 0x01, 0x45, 0x01, 0x58, 0x01, 0x6D, 0x01, 0x82, 0x01, 0x99, 0x01, 0xB2, 0x01, 0xCB, 0x01, 0xE7, 0x02, 0x04, 0x02, 0x22, 0x02, 0x43,
    0x02, 0x65, 0x02, 0x8A, 0x02, 0xB0, 0x02, 0xD9, 0x03, 0x04, 0x03, 0x32, 0x03, 0x63, 0x03, 0x97, 0x03, 0xCD, 0x04, 0x07, 0x04, 0x44, 0x04, 0x85,
    0x04, 0xCA, 0x05, 0x13, 0x05, 0x60, 0x05, 0xB2, 0x06, 0x09, 0x06, 0x65, 0x06, 0xC6, 0x07, 0x2D, 0x07, 0x9A, 0x08, 0x0E, 0x08, 0x89, 0x09, 0x0B,
    0x09, 0x94, 0x0A, 0x26, 0x0A, 0xC1, 0x0B, 0x64, 0x0C, 0x12, 0x0C, 0xCA, 0x0D, 0x8C, 0x0E, 0x5B, 0x0F, 0x35, 0x10, 0x1D, 0x11, 0x12, 0x12, 0x16,
    0x13, 0x29, 0x14, 0x4D, 0x15, 0x82, 0x16, 0xC9, 0x18, 0x24, 0x19, 0x93, 0x1B, 0x19, 0x1C, 0xB5, 0x1E, 0x6A, 0x20, 0x39, 0x22, 0x24, 0x24, 0x2B,
    0x26, 0x52, 0x28, 0x99, 0x2B, 0x03, 0x2D, 0x92, 0x30, 0x48, 0x33, 0x27, 0x36, 0x31, 0x39, 0x6A, 0x3C, 0xD4, 0x40, 0x72, 0x44, 0x47, 0x48, 0x57,
    0x4C, 0xA4, 0x51, 0x32, 0x56, 0x06, 0x5B, 0x24, 0x60, 0x8F, 0x66, 0x4D, 0x6C, 0x62, 0x72, 0xD4, 0x79, 0xA8, 0x80, 0xE4, 0x88, 0x8E, 0x90, 0xAD};

ostream& operator<<(ostream& os, const ProcessStep& ps) {
    ps.print(os);
    return os;
}

ProcessStep* deserializeProcessStep(istream& is) {
    std::string stepType;
    ProcessStep* step = nullptr;
    is >> stepType;
    switch(constHash(stepType.c_str())) {
        case constHash("GetN"):
        step = new ProcessGetN();
        break;
        case constHash("GetT"):
        step = new ProcessGetT();
        break;
        case constHash("GetParam"):
        step = new ProcessGetParam();
        break;
        case constHash("AddN"):
        step = new ProcessAddN();
        break;
        case constHash("Remap"):
        step = new ProcessRemap();
        break;
        case constHash("Clamp"):
        step = new ProcessClamp();
        break;
        case constHash("FetchEnvelope"):
        step = new ProcessFetchEnvelope();
        break;
        case constHash("FetchInstrumentParam"):
        step = new ProcessFetchInstrumentParam();
        break;
        case constHash("AddInstrumentParam"):
        step = new ProcessAddInstrumentParam();
        break;
        case constHash("SendMemWrite"):
        step = new ProcessSendMemWrite();
        break;
        case constHash("LookupPitch"):
        step = new ProcessLookupPitch();
        break;
    }
    if(step != nullptr) {
        step->scan(is);
    }
    return step;
}

void ProcessGetN::exec(ProcessingState* st) {
    st->acc = n;
}

void ProcessGetT::exec(ProcessingState* st) {
    st->acc = st->t;
}

void ProcessGetParam::exec(ProcessingState* st) {
    st->acc = st->params[paramIndex];
}

void ProcessAddN::exec(ProcessingState* st) {
    st->acc += increment;
}

void ProcessRemap::exec(ProcessingState* st) {
    st->acc = ((y - x) * (st->acc - a) / (b - a)) + x;
}

void ProcessClamp::exec(ProcessingState* st) {
    if(st->acc > upper) st->acc = upper;
    if(st->acc < lower) st->acc = lower;
}

void ProcessFetchEnvelope::exec(ProcessingState* st) {
    st->acc = st->instrument.envelopes[envIndex]->evaluate(st->acc, st->t);
}

void ProcessFetchInstrumentParam::exec(ProcessingState* st) {
    st->acc = st->instrument.params[paramIndex].value;
}

void ProcessAddInstrumentParam::exec(ProcessingState* st) {
    st->acc += st->instrument.params[paramIndex].value;
}

void ProcessSendMemWrite::exec(ProcessingState* st) {
    ParamWrite out;
    out.paramIdx = paramIndex;
    if(highByte) {
        out.value = (st->acc >> 8) & 0xFF;
    } else {
        out.value = st->acc & 0xFF;
    }
    st->output.emplace_back(out);
}

void ProcessLookupPitch::exec(ProcessingState* st) {
    int idx = st->acc;
    idx &= 0x7F;
    st->acc = pitch_table[idx*2] << 8;
    st->acc |= pitch_table[(idx*2)+1];
}

void ProcessGetN::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Set Acc to ");
    ImGui::SameLine();
    ImGui::InputInt("##N", &n);
}

void ProcessGetT::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("T -> Acc");
}

void ProcessGetParam::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Get param");
    ImGui::SameLine();
    if(ImGui::BeginCombo("##Event Param", ctx.eventParams[paramIndex].name.c_str())) {
        int i = 0;
        for(const auto& eventParam : ctx.eventParams) {
            if(ImGui::Selectable(eventParam.name.c_str(), paramIndex == i)) {
                paramIndex = i;
            }
            if(paramIndex == i) {
                ImGui::SetItemDefaultFocus();
            }
            ++i;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("from event");
}

void ProcessAddN::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Add");
    ImGui::SameLine();
    ImGui::InputInt("##N", &increment);
}

void ProcessRemap::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Map from (");
    ImGui::SameLine();
    ImGui::InputInt("##Src1", &a);  
    ImGui::SameLine();
    ImGui::TextUnformatted(",");
    ImGui::SameLine();
    ImGui::InputInt("##Src2", &b);
    ImGui::SameLine();
    ImGui::TextUnformatted(") to (");
    ImGui::SameLine();
    ImGui::InputInt("##Dest1", &x);
    ImGui::SameLine();
    ImGui::TextUnformatted(",");
    ImGui::SameLine();
    ImGui::InputInt("##Dest2", &y);
    ImGui::SameLine();
    ImGui::TextUnformatted(")");
}

void ProcessClamp::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Clamp between ");
    ImGui::SameLine();
    ImGui::InputInt("##Lower", &lower);
    ImGui::SameLine();
    ImGui::TextUnformatted("and");
    ImGui::SameLine();
    ImGui::InputInt("##Upper", &upper);
}

void ProcessFetchEnvelope::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Resolve envelope value of");
    ImGui::SameLine();
    if(ImGui::BeginCombo("##Instrument Envelope", ctx.templateInstrument.envelopes[envIndex]->name.c_str())) {
        int i = 0;
        for(const auto& instrumentEnv : ctx.templateInstrument.envelopes) {
            if(ImGui::Selectable(instrumentEnv->name.c_str(), envIndex == i)) {
                envIndex = i;
            }
            if(envIndex == i) {
                ImGui::SetItemDefaultFocus();
            }
            ++i;
        }
        ImGui::EndCombo();
    }
}

void ProcessFetchInstrumentParam::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Get instrument param");
    ImGui::SameLine();
    if(ImGui::BeginCombo("##Instrument Param", ctx.templateInstrument.params[paramIndex].name.c_str())) {
        int i = 0;
        for(const auto& instrumentParam : ctx.templateInstrument.params) {
            if(ImGui::Selectable(instrumentParam.name.c_str(), paramIndex == i)) {
                paramIndex = i;
            }
            if(paramIndex == i) {
                ImGui::SetItemDefaultFocus();
            }
            ++i;
        }
        ImGui::EndCombo();
    }
}

void ProcessAddInstrumentParam::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Add instrument param");
    ImGui::SameLine();
    if(ImGui::BeginCombo("##Instrument Param", ctx.templateInstrument.params[paramIndex].name.c_str())) {
        int i = 0;
        for(const auto& instrumentParam : ctx.templateInstrument.params) {
            if(ImGui::Selectable(instrumentParam.name.c_str(), paramIndex == i)) {
                paramIndex = i;
            }
            if(paramIndex == i) {
                ImGui::SetItemDefaultFocus();
            }
            ++i;
        }
        ImGui::EndCombo();
    }
}

void ProcessSendMemWrite::draw_ui(ProcessStepDrawContext& ctx) {
    ImGui::TextUnformatted("Write to ");
    ImGui::SameLine();
    if(ImGui::BeginCombo("##memVal", ctx.outputs[paramIndex].name.c_str())) {
        int i = 0;
        for(const auto& channelParam : ctx.outputs) {
            if(ImGui::Selectable(channelParam.name.c_str(), paramIndex == i)) {
                paramIndex = i;
            }
            if(paramIndex == i) {
                ImGui::SetItemDefaultFocus();
            }
            ++i;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Checkbox("High byte", &highByte);
}


void ProcessLookupPitch::draw_ui(ProcessStepDrawContext &ctx) {
    ImGui::TextUnformatted("Use Acc to look up pitch value. (TODO: editable lookup table)");
}

void ProcessGetN::print(std::ostream& os) const {
    os << "GetN " << n << std::endl;
}

void ProcessGetT::print(std::ostream& os) const {
    os << "GetT" << std::endl;
}

void ProcessGetParam::print(std::ostream& os) const {
    os << "GetParam " << paramIndex << std::endl;
}

void ProcessAddN::print(std::ostream& os) const {
    os << "AddN " << increment << std::endl;
}

void ProcessRemap::print(std::ostream& os) const {
    os << "Remap " << a << " " << b << " " << x << " " << y << std::endl;
}

void ProcessClamp::print(std::ostream& os) const {
    os << "Clamp " << lower << " " << upper << std::endl;
}

void ProcessFetchEnvelope::print(std::ostream& os) const {
    os << "FetchEnvelope " << envIndex << std::endl;
}

void ProcessFetchInstrumentParam::print(std::ostream& os) const {
    os << "FetchInstrumentParam " << paramIndex << std::endl;
}

void ProcessAddInstrumentParam::print(std::ostream& os) const {
    os << "AddInstrumentParam " << paramIndex << std::endl;
}

void ProcessSendMemWrite::print(std::ostream& os) const {
    os << "SendMemWrite " << paramIndex << " " << highByte << std::endl;
}

void ProcessLookupPitch::print(std::ostream& os) const {
    os << "LookupPitch" << std::endl;
}

void ProcessGetN::scan(std::istream& is) {
    is >> n;
}

void ProcessGetT::scan(std::istream& is) {
    //Nothing to do
}

void ProcessGetParam::scan(std::istream& is) {
    is >> paramIndex;
}

void ProcessAddN::scan(std::istream& is) {
    is >> increment;
}

void ProcessRemap::scan(std::istream& is) {
    is >> a >> b >> x >> y;
}

void ProcessClamp::scan(std::istream& is) {
    is >> lower >> upper;
}

void ProcessFetchEnvelope::scan(std::istream& is) {
    is >> envIndex;
}

void ProcessFetchInstrumentParam::scan(std::istream& is) {
    is >> paramIndex;
}

void ProcessAddInstrumentParam::scan(std::istream& is) {
    is >> paramIndex;
}

void ProcessSendMemWrite::scan(std::istream& is) {
    is >> paramIndex >> highByte;
}

void ProcessLookupPitch::scan(std::istream& is) {
    //Nothing to do
}

