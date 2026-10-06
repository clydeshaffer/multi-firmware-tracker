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

#include "imgui.h"
#include "implot.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"

#include "audio_coprocessor.h"

#define WINDOW_TITLE "My SDL imgui app"

using namespace std;

SDL_Window* mainWindow = NULL;
SDL_Renderer* mainRenderer = NULL;

ImGuiContext* main_imgui_ctx;
ImPlotContext* main_implot_ctx;

AudioCoprocessor* soundcard;

typedef struct track_event {
    uint8_t note;
    uint8_t instrument;
    uint8_t volume;
    uint8_t fx;
} track_event;

vector<vector<track_event>> patternTable;
vector<vector<int>> songTable;

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
    28, 255, 13, 15, 255, 18, 20, 22, 255, 25
};

int octave = 4;

const char* noteNames[] = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "--"
};

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

    //Event handler
    SDL_Event e; 

    int selectedSeqRowIdx = 0;
    int selectedSeqColIdx = 0;
    int selectedPatRowIdx = 0;
    int selectedPatColIdx = 0;
    int numChannels = 4;
    int songLengthInPatterns = 1;
    int patternRows = 64;
    bool recording = false;

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

    while(!quit) {
        //Handle events on queue
        while( SDL_PollEvent( &e ) != 0 )
        {
            //User requests quit
            if( e.type == SDL_QUIT )
            {
                quit = true;
            }
            if(recording) {
                if(e.type == SDL_KEYDOWN) {
                    int noteNum = 255;
                    if((e.key.keysym.sym >= SDLK_a) && (e.key.keysym.sym <= SDLK_z)) {
                        noteNum = keynotes_az[e.key.keysym.sym - SDLK_a];
                    } else if((e.key.keysym.sym >= SDLK_0) && (e.key.keysym.sym <= SDLK_9)) {
                        noteNum = keynotes_09[e.key.keysym.sym - SDLK_0];
                    }
                    if(noteNum != 255) {
                        noteNum += (octave+1)*12;
                        patternTable.at(selectedPatColIdx).at(selectedPatRowIdx).note = noteNum;
                        patternTable.at(selectedPatColIdx).at(selectedPatRowIdx).instrument = 0;
                        ++selectedPatRowIdx;
                        soundcard->ram_write(0x10, pitch_table[noteNum*2]);
                        soundcard->ram_write(0x20, pitch_table[(noteNum*2)+1]);
                        soundcard->ram_write(0x11, pitch_table[noteNum*2]);
                        soundcard->ram_write(0x21, pitch_table[(noteNum*2)+1]);
                        soundcard->ram_write(0x12, pitch_table[noteNum*2]);
                        soundcard->ram_write(0x22, pitch_table[(noteNum*2)+1]);
                        soundcard->ram_write(0x13, pitch_table[noteNum*2]);
                        soundcard->ram_write(0x23, pitch_table[(noteNum*2)+1]);
                        soundcard->ram_write(0x30, 128);
                        soundcard->ram_write(0x31, 128);
                        soundcard->ram_write(0x32, 0);
                        soundcard->ram_write(0x33, 0);
                    }
                }
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
                    sprintf(cellLabel, "%02x", songTable.at(tableColIdx).at(tableRowIdx));
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
                    track_event te = patternTable.at(tableColIdx).at(tableRowIdx);
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
        ImGui::End();

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(mainRenderer);
    }
}