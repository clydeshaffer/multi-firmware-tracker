#include "SDL_inc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <time.h>
#include <fstream>
#include <cstring>
#include <filesystem>
#include <vector>
#include <thread>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>

#include "imgui.h"
#include "implot.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "audio_coprocessor.h"

#include "channel.h"

#define WINDOW_TITLE "My SDL imgui app"

using namespace std;

SDL_Window* mainWindow = NULL;
SDL_Renderer* mainRenderer = NULL;

ImGuiContext* main_imgui_ctx;
ImPlotContext* main_implot_ctx;

AudioCoprocessor* soundcard;

ChannelTemplate mainChannelTemplate;
vector<Channel> channelStates;


int selectedSeqRowIdx = 0;
int selectedSeqColIdx = 0;
int selectedPatRowIdx = 0;
int selectedPatColIdx = 0;
int numChannels = 4;
int songLengthInPatterns = 1;

int patternLengthInRows = 32;
bool recording = false;
bool playing = false;
int playingStepTimer = 0;
int playingStepTime = 15;

#define PITCH_TABLE_LENGTH 216
uint8_t pitch_table[PITCH_TABLE_LENGTH] = {
    0x00, 0x4D, 0x00, 0x51, 0x00, 0x56, 0x00, 0x5B, 0x00, 0x61, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x73, 0x00, 0x7A, 0x00, 0x81, 0x00, 0x89, 0x00, 0x91,
    0x00, 0x99, 0x00, 0xA2, 0x00, 0xAC, 0x00, 0xB6, 0x00, 0xC1, 0x00, 0xCD, 0x00, 0xD9, 0x00, 0xE6, 0x00, 0xF3, 0x01, 0x02, 0x01, 0x11, 0x01, 0x21,
    0x01, 0x33, 0x01, 0x45, 0x01, 0x58, 0x01, 0x6D, 0x01, 0x82, 0x01, 0x99, 0x01, 0xB2, 0x01, 0xCB, 0x01, 0xE7, 0x02, 0x04, 0x02, 0x22, 0x02, 0x43,
    0x02, 0x65, 0x02, 0x8A, 0x02, 0xB0, 0x02, 0xD9, 0x03, 0x04, 0x03, 0x32, 0x03, 0x63, 0x03, 0x97, 0x03, 0xCD, 0x04, 0x07, 0x04, 0x44, 0x04, 0x85,
    0x04, 0xCA, 0x05, 0x13, 0x05, 0x60, 0x05, 0xB2, 0x06, 0x09, 0x06, 0x65, 0x06, 0xC6, 0x07, 0x2D, 0x07, 0x9A, 0x08, 0x0E, 0x08, 0x89, 0x09, 0x0B,
    0x09, 0x94, 0x0A, 0x26, 0x0A, 0xC1, 0x0B, 0x64, 0x0C, 0x12, 0x0C, 0xCA, 0x0D, 0x8C, 0x0E, 0x5B, 0x0F, 0x35, 0x10, 0x1D, 0x11, 0x12, 0x12, 0x16,
    0x13, 0x29, 0x14, 0x4D, 0x15, 0x82, 0x16, 0xC9, 0x18, 0x24, 0x19, 0x93, 0x1B, 0x19, 0x1C, 0xB5, 0x1E, 0x6A, 0x20, 0x39, 0x22, 0x24, 0x24, 0x2B,
    0x26, 0x52, 0x28, 0x99, 0x2B, 0x03, 0x2D, 0x92, 0x30, 0x48, 0x33, 0x27, 0x36, 0x31, 0x39, 0x6A, 0x3C, 0xD4, 0x40, 0x72, 0x44, 0x47, 0x48, 0x57,
    0x4C, 0xA4, 0x51, 0x32, 0x56, 0x06, 0x5B, 0x24, 0x60, 0x8F, 0x66, 0x4D, 0x6C, 0x62, 0x72, 0xD4, 0x79, 0xA8, 0x80, 0xE4, 0x88, 0x8E, 0x90, 0xAD};

int keynotes_az[26] = {
    255, 7, 4, 3, 16, 255, 6, 8, 24, 10, 255, 255, 11, 9, 26, 28, 12, 17, 1, 19, 23, 5, 14, 2, 21, 0
};

int keynotes_09[10] = {
    27, 255, 13, 15, 255, 18, 20, 22, 255, 25
};

int octave = 4;
int selectedInstrumentIdx = 0;

const char* noteNames[] = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "--"
};

void runWriteList(vector<MemWrite> writes) {
    for(auto& mw : writes) {
        soundcard->ram_write(mw.address, mw.value);
    }
}

void playNote(int channel, int note, int instrument) {
    vector<int> params;
    params.emplace_back(note);
    params.emplace_back(0);
    auto noteHitWrites = channelStates[channel].processNoteHit(channel, params, instrument);
    auto noteTickWrites = channelStates[channel].processNoteTick(channel);

    runWriteList(noteHitWrites);
    runWriteList(noteTickWrites);
}

void tickChannels() {
    for(int i = 0; i < numChannels; ++i) {
        auto noteTickWrites = channelStates[i].processNoteTick(i);
        runWriteList(noteTickWrites);
    }
}

void tickPlayback() {
    if(playing) {
        playingStepTimer ++;
        if(playingStepTimer >= playingStepTime) {
            playingStepTimer -= playingStepTime;
            for(int col = 0; col < numChannels; ++col) {
                vector<int> te = channelStates[col].patterns.getEvent(channelStates[col].patterns.patternSequence[selectedSeqRowIdx], selectedPatRowIdx);
                if(te[0] != 255) {
                    playNote(col, te[0], te[1]);
                }
            }
            ++selectedPatRowIdx;
            if(selectedPatRowIdx >= patternLengthInRows) {
                ++selectedSeqRowIdx;
                if(selectedSeqRowIdx >= songLengthInPatterns) {
                    selectedSeqRowIdx = 0;
                }
                selectedPatRowIdx = 0;
            }
        }
    }
}

void musicPlaybackWorkerLoop(std::atomic<bool>& quit) {
    using namespace std::chrono;
    constexpr duration<double> frameDuration(1.0 / 60.0);

    auto nextFrameTime = steady_clock::now();

    while(!quit.load()) {

        tickChannels();
        tickPlayback();

        nextFrameTime += duration_cast<steady_clock::duration>(frameDuration);
        std::this_thread::sleep_until(nextFrameTime);
    }
}

int sprintfNoteName(char* str, int note) {
    return sprintf(str, "%s%01d", noteNames[note % 12], ((int) (note / 12)) - 1);
}

void sequenceRowPopup(int row) {
    if(ImGui::BeginPopupContextItem("row_context_menu")) {
        if(ImGui::MenuItem("Insert Frame")) {
            for(auto& channel : channelStates) {
                int num = channel.patterns.paramPatterns[0].size();
                if(channel.patterns.paramPatterns[0].size() <= num) {
                    channel.patterns.addPattern();
                }
                channel.patterns.patternSequence.insert(channel.patterns.patternSequence.begin() + row + 1, num);
            }
            ++songLengthInPatterns;
        }
        if(ImGui::MenuItem("Duplicate")) {
            for(auto& channel : channelStates) {
                int num = channel.patterns.patternSequence[row];
                channel.patterns.patternSequence.insert(channel.patterns.patternSequence.begin() + row, num);
            }
            ++songLengthInPatterns;
        }
        if(ImGui::MenuItem("Delete")) {
            for(auto& channel : channelStates) {
                channel.patterns.patternSequence.erase(channel.patterns.patternSequence.begin() + row);
            }
            --songLengthInPatterns;
        }
        ImGui::EndPopup();
    }
}

int main(int argC, char* argV[]) {
    SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);

    mainWindow = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);
    main_imgui_ctx = ImGui::CreateContext();
	main_implot_ctx = ImPlot::CreateContext();
    ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForSDLRenderer(mainWindow, mainRenderer);
	ImGui_ImplSDLRenderer2_Init(mainRenderer);

    //Main loop flag
    bool quit = false;
    std::atomic<bool> endMusicThread(false);

    //Event handler
    SDL_Event e; 

    std::ifstream fin("fm_firmware.ini");

    fin >> numChannels;

    for(int i = 0; i < numChannels; ++i) {
        channelStates.emplace_back(Channel(mainChannelTemplate));    
    }

    fin >> mainChannelTemplate;
    fin.close();

    mainChannelTemplate.instruments.emplace_back(mainChannelTemplate.templateInstrument.create());

    soundcard = new AudioCoprocessor();
    AudioCoprocessor::singleton_acp_state->isEmulationPaused = false;
    soundcard->register_write(ACP_RESET, 1);
    soundcard->register_write(ACP_RATE, 0xFF);

    soundcard->ram_write(0x30, 128);
    soundcard->ram_write(0x31, 128);
    soundcard->ram_write(0x32, 128);
    soundcard->ram_write(0x33, 128);
    soundcard->ram_write(0x34, 128);
    soundcard->ram_write(0x35, 128);
    soundcard->ram_write(0x36, 128);
    soundcard->ram_write(0x37, 128);
    soundcard->ram_write(0x38, 128);
    soundcard->ram_write(0x39, 128);
    soundcard->ram_write(0x3A, 128);
    soundcard->ram_write(0x3B, 128);
    soundcard->ram_write(0x3C, 128);
    soundcard->ram_write(0x3D, 128);
    soundcard->ram_write(0x3E, 128);
    soundcard->ram_write(0x3F, 128);
    uint64_t ticks = SDL_GetTicks64();

    std::thread musicPlaybackThread(musicPlaybackWorkerLoop, std::ref(endMusicThread));

    while(!quit) {
        uint64_t lastTicks = ticks;
        ticks = SDL_GetTicks64();
        uint64_t deltaTicks = ticks - lastTicks;
        //Handle events on queue
        while( SDL_PollEvent( &e ) != 0 )
        {
            //User requests quit
            if( e.type == SDL_QUIT )
            {
                quit = true;
            }
            if(e.type == SDL_KEYDOWN) {
                if(recording) {
                    int noteNum = 255;
                    if((e.key.keysym.sym >= SDLK_a) && (e.key.keysym.sym <= SDLK_z)) {
                        noteNum = keynotes_az[e.key.keysym.sym - SDLK_a];
                    } else if((e.key.keysym.sym >= SDLK_0) && (e.key.keysym.sym <= SDLK_9)) {
                        noteNum = keynotes_09[e.key.keysym.sym - SDLK_0];
                    } else {
                        vector<int> p;
                        switch(e.key.keysym.sym) {
                            case SDLK_DELETE:
                            p.resize(mainChannelTemplate.eventParams.size());
                            p[0] = 255;
                            channelStates[selectedPatColIdx].patterns.writeEvent(p, channelStates[selectedPatColIdx].patterns.patternSequence[selectedSeqRowIdx], selectedPatRowIdx);
                            if(!playing) {
                                ++selectedPatRowIdx;
                            }
                            break;
                            case SDLK_BACKSPACE:
                            if(selectedPatRowIdx > 0) {
                                if(!playing) {
                                    --selectedPatRowIdx;
                                }
                                p.resize(mainChannelTemplate.eventParams.size());
                                p[0] = 255;
                                channelStates[selectedPatColIdx].patterns.writeEvent(p, channelStates[selectedPatColIdx].patterns.patternSequence[selectedSeqRowIdx], selectedPatRowIdx);
                            }
                            break;
                        }
                    }
                    if(noteNum != 255) {
                        noteNum += (octave+1)*12;
                        vector<int> p;
                        p.resize(mainChannelTemplate.eventParams.size());
                        p[0] = noteNum;
                        p[1] = selectedInstrumentIdx;
                        channelStates[selectedPatColIdx].patterns.writeEvent(p, channelStates[selectedPatColIdx].patterns.patternSequence[selectedSeqRowIdx], selectedPatRowIdx);
                        if(!playing) {
                            ++selectedPatRowIdx;
                        }
                        playNote(selectedPatColIdx, noteNum, selectedInstrumentIdx);
                    }
                }
                switch (e.key.keysym.sym) {
                    case SDLK_DOWN:
                    ++selectedPatRowIdx;
                    if(selectedPatRowIdx == patternLengthInRows) {
                        selectedPatRowIdx = 0;
                    }
                    break;
                    case SDLK_UP:
                    if(selectedPatRowIdx == 0) {
                        selectedPatRowIdx = patternLengthInRows;
                    }
                    --selectedPatRowIdx;
                    break;
                    case SDLK_SPACE:
                    recording = !recording;
                    break;
                    case SDLK_RETURN:
                    playing = !playing;
                    if(playing) {
                        playingStepTimer = 0;
                    }
                    selectedPatRowIdx = 0;
                    break;
                    case SDLK_RIGHT:
                    ++selectedPatColIdx;
                    if(selectedPatColIdx == numChannels) {
                        selectedPatColIdx = 0;
                    }
                    break;
                    case SDLK_LEFT:
                     if(selectedPatColIdx == 0) {
                        selectedPatColIdx = numChannels;
                    }
                    --selectedPatColIdx;
                    break;
                    case SDLK_KP_MULTIPLY:
                    if(octave < 8)
                        ++octave;
                    break;
                    case SDLK_KP_DIVIDE:
                    if(octave > 0)
                        --octave;
                    break;
                }
                if(selectedPatRowIdx < 0) selectedPatRowIdx = patternLengthInRows - 1;
                else if(selectedPatRowIdx == patternLengthInRows) selectedPatRowIdx = 0;
            }
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        ImGui::SetCurrentContext(main_imgui_ctx);
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Main", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );

        if(ImGui::Button("Exit")) {
            quit = true;
        }

        if(recording) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.7f, 1.0f));
        }
        if(ImGui::Button("rec")) {
            recording = !recording;
        }
        ImGui::PopStyleColor();

        if(playing) {
            if(ImGui::Button("stop")) {
                playing = false;
            }
        } else {
            if(ImGui::Button("play")) {
                playing = true;
                playingStepTimer = 0;
            }
        }

        ImGui::InputInt("Frames per step", &playingStepTime, 1, 10);
        ImGui::InputInt("Octave", &octave, 1, 1);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::BeginChild("sequences", ImVec2(0, 128), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysUseWindowPadding);

        if(ImGui::Button("-##DecPattern")) {
            if(channelStates[selectedSeqColIdx].patterns.patternSequence[selectedSeqRowIdx]) {
                --channelStates[selectedSeqColIdx].patterns.patternSequence[selectedSeqRowIdx];
            }
        }
        ImGui::SameLine();
        if(ImGui::Button("+##IncPattern")) {
            if(channelStates[selectedSeqColIdx].patterns.patternSequence[selectedSeqRowIdx] >= (channelStates[selectedSeqColIdx].patterns.paramPatterns[0].size() - 1)) {
                channelStates[selectedSeqColIdx].patterns.addPattern();
            }
            ++channelStates[selectedSeqColIdx].patterns.patternSequence[selectedSeqRowIdx];
        }

        if(ImGui::BeginTable("sequenceView", numChannels+2, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
            for(int tableRowIdx = 0; tableRowIdx < songLengthInPatterns; ++tableRowIdx) { 
                ImGui::PushID(tableRowIdx);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                char label[16];
                char cellLabel[16];
                sprintf(label, "%02x", tableRowIdx);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.3f, 1.0f));
                if(ImGui::Selectable(label, tableRowIdx == selectedSeqRowIdx)) {
                    selectedSeqRowIdx = tableRowIdx;
                }
                ImGui::PopStyleColor();
                sequenceRowPopup(tableRowIdx);

                if(tableRowIdx == selectedSeqRowIdx) {
                    ImU32 bg_color = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 1.0f));
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg_color);
                }
                for(int tableColIdx = 0; tableColIdx < numChannels; ++tableColIdx) {
                    ImGui::PushID(tableColIdx);
                    ImGui::TableSetColumnIndex(tableColIdx+1);
                    //ImGui::TextUnformatted("00");
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                    if((tableColIdx == selectedSeqColIdx) && (tableRowIdx == selectedSeqRowIdx)) {
                        ImU32 bg_color = ImGui::GetColorU32(ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bg_color);
                    }
                    sprintf(cellLabel, "%02x", channelStates[tableColIdx].patterns.patternSequence[tableRowIdx]);
                    if(ImGui::Selectable(cellLabel, (tableRowIdx == selectedSeqRowIdx) && (tableColIdx == selectedSeqColIdx))) {
                        selectedSeqRowIdx = tableRowIdx;
                        selectedSeqColIdx = tableColIdx;
                    }
                    ImGui::PopStyleColor();
                    sequenceRowPopup(tableRowIdx);
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("  ");
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::SameLine();
        if(ImGui::BeginChild("instruments", ImVec2(128, 128), ImGuiChildFlags_Border)) {
            ImGui::TextUnformatted("Instruments");
            ImGui::SameLine();
            if(ImGui::Button("+##AddInstrument")) {
                mainChannelTemplate.instruments.emplace_back(mainChannelTemplate.templateInstrument.create());
            }
            int instrIdx = 0;
            for(auto& instrument : mainChannelTemplate.instruments) {
                ImGui::PushID(instrIdx);
                if(ImGui::Selectable(instrument.name.c_str(), instrIdx == selectedInstrumentIdx)) {
                    selectedInstrumentIdx = instrIdx;
                }
                instrIdx++;
                ImGui::PopID();
            }
        }
        ImGui::EndChild();


        ImGui::BeginChild("tracks", ImVec2(0, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
        if(ImGui::BeginTable("trackerView", numChannels+1, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
            for(int tableRowIdx = 0; tableRowIdx < patternLengthInRows; ++tableRowIdx) { 
                ImGui::PushID(tableRowIdx);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                char label[16];
                char cellLabel[64];
                char patfmt[8];
                int paramCnt = mainChannelTemplate.eventParams.size();
                size_t cellLabelOffset = 0;
                sprintf(label, "%02x", tableRowIdx);
                if(ImGui::Selectable(label, tableRowIdx == selectedPatRowIdx, 0)) {
                    selectedPatRowIdx = tableRowIdx;
                }

                if(tableRowIdx == selectedPatRowIdx) {
                    ImU32 bg_color = recording ? ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 1.0f)) : ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 1.0f));
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg_color);
                } else if(!(tableRowIdx & 0x7)) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 1.0f)));
                }
                for(int tableColIdx = 0; tableColIdx < numChannels; ++tableColIdx) {
                    cellLabelOffset = 0;
                    ImGui::PushID(tableColIdx);
                    ImGui::TableSetColumnIndex(tableColIdx+1);
                    //ImGui::TextUnformatted("--- -- - ---");
                    vector<int> te = channelStates[tableColIdx].patterns.getEvent(channelStates[tableColIdx].patterns.patternSequence[selectedSeqRowIdx], tableRowIdx);
                    if(te[0] == 255) {
                        int wid = mainChannelTemplate.eventParams[0].displayWidth;
                        for(int i = 0; i < wid; ++i) {
                            cellLabel[cellLabelOffset++] = '-';
                        }
                        for(int paramIdx = 1; paramIdx < mainChannelTemplate.eventParams.size(); ++paramIdx) {
                            int wid = mainChannelTemplate.eventParams[paramIdx].displayWidth;
                            cellLabel[cellLabelOffset++] = ' ';
                            for(int i = 0; i < wid; ++i) {
                                cellLabel[cellLabelOffset++] = '-';
                            }
                            
                        }
                    } else {
                        cellLabelOffset = sprintfNoteName(cellLabel, te[0]);
                        for(int paramIdx = 1; paramIdx < paramCnt; ++paramIdx) {
                            sprintf(patfmt, " %%0%dx", mainChannelTemplate.eventParams[paramIdx].displayWidth);
                            cellLabelOffset += sprintf(&cellLabel[cellLabelOffset], patfmt, te[paramIdx]);
                        }
                    }
                    if(ImGui::Selectable(cellLabel,  (tableRowIdx == selectedPatRowIdx) && (tableColIdx == selectedPatColIdx), 0)) {
                        selectedPatRowIdx = tableRowIdx;
                        selectedPatColIdx = tableColIdx;
 
                    }
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleVar(2);

        ImGui::Begin("firmware config");
        ImGui::TextUnformatted("configure firmware properties here");
        if(ImGui::InputInt("Channels", &numChannels, 1, 1, 0)) {
            if(numChannels < 1) numChannels = 1;
            if(numChannels >= channelStates.size()) {
                channelStates.emplace_back(Channel(mainChannelTemplate)); 
            }
        }
        mainChannelTemplate.renderConfigUI();
        ImGui::End();


        ImGui::Begin("instrument config");
        if((selectedInstrumentIdx >= 0) && (selectedInstrumentIdx < mainChannelTemplate.instruments.size())) {
            mainChannelTemplate.instruments[selectedInstrumentIdx].renderConfigUI();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(mainRenderer);
    }
    endMusicThread.store(true);
    if(musicPlaybackThread.joinable()) {
        musicPlaybackThread.join();
    }

    std::ofstream fout("fm_firmware.ini");
    fout << numChannels << std::endl;
    fout << mainChannelTemplate;
    fout << std::endl;

    return 0;
}