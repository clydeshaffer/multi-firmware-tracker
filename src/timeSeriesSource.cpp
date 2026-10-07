#include "timeSeriesSource.h"
#include "imgui.h"

TimeSeriesSource* ArrayEnvSourceSpec::create() {
    return new ArrayEnvSource(this);
}

ArrayEnvSource::ArrayEnvSource(ArrayEnvSourceSpec* spec) : parent(spec) {
    envelopeData.resize(16);
    for(int i = 0; i < 16; i++) {
        envelopeData[i] = 16 - i;
    }
    envelopeData[15] = 0;
}

int ArrayEnvSource::evaluate(int initial, int t) {
    if(envelopeData.size() == 0) return 0;
    t += initial;
    if(t >= envelopeData.size()) return envelopeData.back();
    if(t < 0) return envelopeData.front();
    return envelopeData[t];
}

void ArrayEnvSource::draw_instrument_ui() {
    ImGui::TextUnformatted(parent->name);
    for(int i = 0; i < envelopeData.size(); ++i) {
        ImGui::PushID(i);
        if(i > 0) ImGui::SameLine();
        ImGui::VSliderScalar("##envSlider",ImVec2(8,64), ImGuiDataType_U8, &(envelopeData[i]), &parent->min, &parent->max, nullptr, ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopID();
    }
}

void ArrayEnvSourceSpec::draw_setup_ui() {
    ImGui::TextUnformatted("Name:");
    ImGui::SameLine();
    ImGui::InputText("##Name", name, 64);
    ImGui::TextUnformatted("Range");
    ImGui::SameLine();
    ImGui::InputInt("##Min", &min);
    ImGui::SameLine();
    ImGui::InputInt("##Max", &max);
}

TimeSeriesSourceType ArrayEnvSource::type() {
    return TimeSeriesSourceType::ARRAY;
}