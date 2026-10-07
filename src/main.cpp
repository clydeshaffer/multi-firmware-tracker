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


typedef struct track_event {
    uint8_t note;
    uint8_t instrument;
    uint8_t volume;
    uint8_t fx;
} track_event;

vector<vector<track_event>> patternTable;
vector<vector<int>> songTable;

int selectedSeqRowIdx = 0;
int selectedSeqColIdx = 0;
int selectedPatRowIdx = 0;
int selectedPatColIdx = 0;
int numChannels = 4;
int songLengthInPatterns = 1;

int channelTimes[32];

int patternRows = 32;
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

const char* noteNames[] = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "--"
};

#define MAX_ENVELOPE_LENGTH 256
vector<uint8_t> envelopeLengths;
vector<std::array<uint8_t, MAX_ENVELOPE_LENGTH>> envelopes;

void runWriteList(vector<MemWrite> writes) {
    for(auto& mw : writes) {
        soundcard->ram_write(mw.address, mw.value);
    }
}

void playNote(int channel, int note) {
   /* note -= 36;
    soundcard->ram_write(0x10 + (channel*4), pitch_table[note*2]);
    soundcard->ram_write(0x20 + (channel*4), pitch_table[(note*2)+1]);
    soundcard->ram_write(0x11 + (channel*4), pitch_table[(note+12)*2]);
    soundcard->ram_write(0x21 + (channel*4), pitch_table[((note+12)*2)+1]);
    soundcard->ram_write(0x12 + (channel*4), pitch_table[(note+12)*2]);
    soundcard->ram_write(0x22 + (channel*4), pitch_table[((note+12)*2)+1]);
    soundcard->ram_write(0x13 + (channel*4), pitch_table[note*2]);
    soundcard->ram_write(0x23 + (channel*4), pitch_table[(note*2)+1]);
    soundcard->ram_write(0x30 + (channel*4), envelopes[0][0]);
    soundcard->ram_write(0x31 + (channel*4), envelopes[1][0]);
    soundcard->ram_write(0x32 + (channel*4), envelopes[2][0]);
    soundcard->ram_write(0x33 + (channel*4), envelopes[3][0]);
    channelTimes[channel] = 0;*/
    vector<int> params;
    params.emplace_back(note);
    params.emplace_back(0);
    auto noteHitWrites = channelStates[channel].processNoteHit(channel, params, 0);
    auto noteTickWrites = channelStates[channel].processNoteTick(channel);

    runWriteList(noteHitWrites);
    runWriteList(noteTickWrites);
}

void tickChannels() {
    for(int i = 0; i < numChannels; ++i) {
        /*if(channelTimes[i] < MAX_ENVELOPE_LENGTH) ++channelTimes[i];
        for(int envIdx = 0; envIdx < 4; ++envIdx) {
            if(channelTimes[i] < envelopeLengths[envIdx]) {
                soundcard->ram_write(0x30 + (i*4) + envIdx, envelopes[envIdx][channelTimes[i]]);
            }
        }*/
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
                int noteNum = patternTable[col][selectedPatRowIdx].note;
                if(noteNum != 255) {
                    playNote(col, noteNum);
                }
            }
            ++selectedPatRowIdx;
            if(selectedPatRowIdx >= patternRows) {
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

    //envelopes.emplace_back(std::array<uint8_t,MAX_ENVELOPE_LENGTH>());
    //envelopes.emplace_back(std::array<uint8_t,MAX_ENVELOPE_LENGTH>());
    //envelopes.emplace_back(std::array<uint8_t,MAX_ENVELOPE_LENGTH>());
    //envelopes.emplace_back(std::array<uint8_t,MAX_ENVELOPE_LENGTH>());

    channelStates.emplace_back(Channel(mainChannelTemplate));
    channelStates.emplace_back(Channel(mainChannelTemplate));
    channelStates.emplace_back(Channel(mainChannelTemplate));
    channelStates.emplace_back(Channel(mainChannelTemplate));

    mainChannelTemplate.templateInstrument.name = "FM Instrument";
    ArrayEnvSourceSpec templateArrayEnv[4];
    NamedInstrumentParamTemplate niptOpNoteAdj[4];
    for(int i = 0; i < 4; ++i) {
        templateArrayEnv[i].min = 0;
        templateArrayEnv[i].max = 16;
        sprintf(templateArrayEnv[i].name, "Operator %d Volume", i);
        mainChannelTemplate.templateInstrument.envelopes.emplace_back(&templateArrayEnv[i]);

        
        sprintf(niptOpNoteAdj[i].name, "Op %d Note Offset", i);
        niptOpNoteAdj[i].min = -128;
        niptOpNoteAdj[i].max = 128;
        mainChannelTemplate.templateInstrument.params.emplace_back(niptOpNoteAdj[i]);
    }

    mainChannelTemplate.instruments.emplace_back(mainChannelTemplate.templateInstrument.create());

    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op1 Pitch MSB", 0x10));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op2 Pitch MSB", 0x11));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op3 Pitch MSB", 0x12));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op4 Pitch MSB", 0x13));

    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op1 Pitch LSB", 0x20));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op2 Pitch LSB", 0x21));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op3 Pitch LSB", 0x22));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op4 Pitch LSB", 0x23));

    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op1 Amplitude", 0x30));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op2 Amplitude", 0x31));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op3 Amplitude", 0x32));
    mainChannelTemplate.outputs.emplace_back(NamedOffset("Op4 Amplitude", 0x33));

    mainChannelTemplate.eventParams.emplace_back(EventParam("Note", 1, 3));
    mainChannelTemplate.eventParams.emplace_back(EventParam("Instrument", 1, 2));

    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessGetParam(0));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessAddInstrumentParam(0));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessLookupPitch());
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(0, true));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(4, false));

    
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessGetParam(0));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessAddInstrumentParam(1));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessLookupPitch());
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(1, true));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(5, false));
    
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessGetParam(0));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessAddInstrumentParam(2));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessLookupPitch());
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(2, true));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(6, false));
    
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessGetParam(0));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessAddInstrumentParam(3));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessLookupPitch());
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(3, true));
    mainChannelTemplate.noteHitSteps.emplace_back(new ProcessSendMemWrite(7, false));

    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessGetN(0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessFetchEnvelope(0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessRemap(0, 16, 128, 0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessSendMemWrite(8, false));

    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessGetN(0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessFetchEnvelope(1));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessRemap(0, 16, 128, 0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessSendMemWrite(9, false));

    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessGetN(0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessFetchEnvelope(2));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessRemap(0, 16, 128, 0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessSendMemWrite(10, false));

    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessGetN(0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessFetchEnvelope(3));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessRemap(0, 16, 128, 0));
    mainChannelTemplate.noteSustainSteps.emplace_back(new ProcessSendMemWrite(11, false));

    envelopes.resize(4);
    envelopeLengths.resize(4);


    envelopes[0].fill(128);
    envelopes[1].fill(128);
    envelopes[2].fill(128);
    envelopes[3].fill(128);
    envelopeLengths[0] = 1;
    envelopeLengths[1] = 4;
    envelopeLengths[2] = 16;
    envelopeLengths[3] = 16;

    for(int i = 0; i < 15; ++i) {
        envelopes[1][i] = i * 32;
        envelopes[2][i] = i * 8;
        envelopes[3][i] = i * 8;
    }
    envelopes[1][3] = 128;
    envelopes[2][15] = 128;
    envelopes[3][15] = 128;

    for(int c = 0; c < numChannels; ++c) { 
        vector<track_event> initial_blank_pattern;
        for(int r = 0; r < patternRows; ++r) {
            track_event te;
            te.note = 255;
            te.volume = 255;
            te.instrument = 255;
            te.fx = 255;
            initial_blank_pattern.emplace_back(te);
        }   
        patternTable.emplace_back(initial_blank_pattern);

        vector<int> initial_frame;
        initial_frame.emplace_back(0);
        songTable.emplace_back(initial_frame);

        channelTimes[c] = MAX_ENVELOPE_LENGTH;
    }

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
                    }
                    if(noteNum != 255) {
                        noteNum += (octave+1)*12;
                        patternTable[selectedPatColIdx][selectedPatRowIdx].note = noteNum;
                        patternTable[selectedPatColIdx][selectedPatRowIdx].instrument = 0;
                        if(!playing) {
                            ++selectedPatRowIdx;
                        }
                        playNote(selectedPatColIdx, noteNum);
                    } else if(e.key.keysym.sym == SDLK_DELETE) {
                        patternTable[selectedPatColIdx][selectedPatRowIdx].note = 255;
                        if(!playing) {
                            ++selectedPatRowIdx;
                        }
                    }
                }
                if(e.key.keysym.sym == SDLK_DOWN) {
                    ++selectedPatRowIdx;
                    if(selectedPatRowIdx == patternRows) {
                        selectedPatRowIdx = 0;
                    }
                } else if(e.key.keysym.sym == SDLK_UP) {
                    if(selectedPatRowIdx == 0) {
                        selectedPatRowIdx = patternRows;
                    }
                    --selectedPatRowIdx;
                    
                } else if(e.key.keysym.sym == SDLK_SPACE) {
                    recording = !recording;
                } else if(e.key.keysym.sym == SDLK_RETURN) {
                    playing = !playing;
                    if(playing) {
                        playingStepTimer = 0;
                    }
                    selectedPatRowIdx = 0;
                } else if(e.key.keysym.sym == SDLK_RIGHT) {
                    ++selectedPatColIdx;
                    if(selectedPatColIdx == numChannels) {
                        selectedPatColIdx = 0;
                    }
                } else if(e.key.keysym.sym == SDLK_LEFT) {
                    if(selectedPatColIdx == 0) {
                        selectedPatColIdx = numChannels;
                    }
                    --selectedPatColIdx;
                }
                if(selectedPatRowIdx < 0) selectedPatRowIdx = patternRows - 1;
                else if(selectedPatRowIdx == patternRows) selectedPatRowIdx = 0;
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

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::BeginChild("sequences", ImVec2(0, 128), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysUseWindowPadding);
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
                    sprintf(cellLabel, "%02x", songTable[tableColIdx][tableRowIdx]);
                    if(ImGui::Selectable(cellLabel, (tableRowIdx == selectedSeqRowIdx) && (tableColIdx == selectedSeqColIdx))) {
                        selectedSeqRowIdx = tableRowIdx;
                        selectedSeqColIdx = tableColIdx;
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("  ");
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::BeginChild("tracks", ImVec2(0, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
        if(ImGui::BeginTable("trackerView", numChannels+1, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {
            for(int tableRowIdx = 0; tableRowIdx < patternRows; ++tableRowIdx) { 
                ImGui::PushID(tableRowIdx);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                char label[16];
                char cellLabel[16];
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
                    ImGui::PushID(tableColIdx);
                    ImGui::TableSetColumnIndex(tableColIdx+1);
                    //ImGui::TextUnformatted("--- -- - ---");
                    track_event te = patternTable[tableColIdx][tableRowIdx];
                    if(te.note == 255) {
                        sprintf(cellLabel, "--- -- %01x %02x", te.volume & 15, te.fx);
                    } else {
                        sprintf(cellLabel, "%s%01d %02x %01x %02x", noteNames[te.note % 12], ((int) (te.note / 12)) - 1, te.instrument, te.volume & 15, te.fx);
                    }

                    if(te.volume == 255) {
                        cellLabel[7] = '-';
                    }
                    if(te.fx == 255) {
                        cellLabel[9] = '-';
                        cellLabel[10] = '-';
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
        }
        mainChannelTemplate.renderConfigUI();
        ImGui::End();


        ImGui::Begin("instrument config");
        int i = 0;
        for(auto& instrument : mainChannelTemplate.instruments) {
            ImGui::PushID(i++);
            instrument.renderConfigUI();
            ImGui::PopID();
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
    return 0;
}