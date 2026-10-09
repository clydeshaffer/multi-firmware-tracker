#include "timeSeriesSource.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

unique_ptr<TimeSeriesSource> ArrayEnvSourceSpec::create() {
    return make_unique<ArrayEnvSource>(this);
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
    ImGui::TextUnformatted(parent->name.c_str());
    if(ImGui::Button("-")) {
		if(envelopeData.size() > 1) {
				envelopeData.pop_back();
		}
    }
    ImGui::SameLine();
    for(int i = 0; i < envelopeData.size(); ++i) {
        ImGui::PushID(i);
        if(i > 0) ImGui::SameLine();
        ImGui::VSliderScalar("##envSlider",ImVec2(8,64), ImGuiDataType_U8, &(envelopeData[i]), &parent->min, &parent->max, nullptr, ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopID();
    }
    ImGui::SameLine();
    if(ImGui::Button("+")) {
		int lastVal = envelopeData.back();
		envelopeData.emplace_back();
		envelopeData.back() = lastVal;
    }
}

void ArrayEnvSourceSpec::draw_setup_ui() {
    ImGui::TextUnformatted("Name:");
    ImGui::SameLine();
    ImGui::InputText("##Name", &name);
    ImGui::TextUnformatted("Range");
    ImGui::SameLine();
    ImGui::InputInt("##Min", &min);
    ImGui::SameLine();
    ImGui::InputInt("##Max", &max);
}

TimeSeriesSourceType ArrayEnvSource::type() {
    return TimeSeriesSourceType::ARRAY;
}

void ArrayEnvSourceSpec::print(std::ostream& os) const {
    os << "ARRAY \"" << name << "\" " << min << " " << max << std::endl; 
}

void ArrayEnvSourceSpec::scan(std::istream& is) {
    getQuotedString(is, name) >> min >> max;
}

void ArrayEnvSource::print(std::ostream& os) const {
    os << envelopeData.size();
    for(auto& envSlice : envelopeData) {
        os << " " << envSlice;
    }
    os << std::endl;
}

void ArrayEnvSource::scan(std::istream& is) {
    int arraySize;
    is >> arraySize;
    envelopeData.resize(arraySize);
    for(int i = 0; i < arraySize; ++i) {
        is >> envelopeData[i];
    }
}

ostream& operator<<(ostream& os, const TimeSeriesSourceSpec& tsss) {
    tsss.print(os);
    return os;
}

unique_ptr<TimeSeriesSourceSpec> deserializeTimeSeriesSourceSpec(std::istream& is) {
    std::string typeName;
    is >> typeName;
    unique_ptr<TimeSeriesSourceSpec> newSourceSpec = nullptr;
    if(typeName == "ARRAY") {
        newSourceSpec = make_unique<ArrayEnvSourceSpec>();
    }
    if(newSourceSpec != nullptr) {
        newSourceSpec->scan(is);
    }
    return newSourceSpec;
}

ostream& operator<<(ostream& os, const TimeSeriesSource& tss) {
    tss.print(os);
    return os;
}

