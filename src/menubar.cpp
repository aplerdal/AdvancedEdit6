#include "menubar.hpp"

#include <SDL3/SDL.h>
#include "imgui.h"

#include <fstream>

#include "editor.hpp"
#include "types.h"

#include "tracklist.hpp"
#include <iostream>

std::string MenuBar::get_name(){
    return "#MENUBAR";
}

void MenuBar::update(AppState* as){
    if (debug_open) {
        ImGui::ShowMetricsWindow(&debug_open);
    }
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open ROM", "ctrl+o")) {
                SDL_ShowOpenFileDialog(
                    OpenFileCallback, 
                    as, 
                    as->window, 
                    gbaFileFilter, 
                    2, 
                    NULL, 
                    false
                );
            }
            ImGui::MenuItem("Save ROM", "ctrl+s");
            ImGui::Separator();
            ImGui::MenuItem("Open Project", "ctrl+shift+o");
            ImGui::MenuItem("Save Project", "ctrl+shift+s");
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "ctrl+o")){
                as->editor_ctx.app_result = SDL_APP_SUCCESS;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Undo", "ctrl+z");
            ImGui::MenuItem("Redo", "ctrl+y");
            ImGui::Separator();
            ImGui::MenuItem("Copy", "ctrl+c");
            ImGui::MenuItem("Paste", "ctrl+v");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            for (auto o : as->editor_ctx.scenes){
                if (o->get_name()[0] != '#'){
                    ImGui::MenuItem(o->get_name().c_str(), NULL, &(o->open));
                }
            }
            ImGui::MenuItem("Debug Window", NULL, &debug_open);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Run")){
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void SDLCALL OpenFileCallback(void* userdata, const char* const* filelist, int filter){
    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }


    // Thanks to Loki Astari on stack exchange: https://codereview.stackexchange.com/questions/22901/reading-all-bytes-from-a-file
    std::ifstream ifs(*filelist, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    AppState* as = (AppState*)userdata;
    if (pos < 0x400000) {
        ImGui::BeginPopup("ERROR");
        ImGui::Text("Error reading file: Incorrect file size! Did you open the correct file?");
        ImGui::EndPopup();
        return;
    }
    std::vector<uint8_t> buf(pos);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)&buf[0], pos);

    as->editor_ctx.file = buf;
    as->editor_ctx.file_open = true;
    ifs.close();
    // Load Track pointers
    as->game_ctx.track_table = (TrackTable*)(&as->editor_ctx.file.data()[TRACK_TABLE_ADDRESS]);

    as->game_ctx.eof = as->editor_ctx.file.data() + as->editor_ctx.file.size();
    for (int track = 0; track < TRACK_COUNT; track++) {
        as->game_ctx.tracks[track].track_header = (TrackHeader*)((uint8_t*)as->game_ctx.track_table + as->game_ctx.track_table->track_offsets[track]);
        as->game_ctx.tracks[track].definition_table = (TrackDefinition*)(&as->editor_ctx.file.data()[DEFINITION_TABLE_ADDRESS+track*sizeof(TrackDefinition)]);
        as->game_ctx.tracks[track].ai_header = (AiHeader*)((uint8_t*)as->game_ctx.tracks[track].track_header + as->game_ctx.tracks[track].track_header->ai_offset);
        auto h = as->game_ctx.tracks[track].ai_header;
        as->game_ctx.tracks[track].ai_zones.resize(as->game_ctx.tracks[track].ai_header->count);
        for (int zone_group = 0; zone_group < 3; zone_group++){
            as->game_ctx.tracks[track].ai_targets[zone_group].resize(as->game_ctx.tracks[track].ai_header->count);
        }
        for (int i = 0; i < as->game_ctx.tracks[track].ai_header->count; i++) {
            as->game_ctx.tracks[track].ai_zones[i] = ((AiZone*)((uint8_t*)as->game_ctx.tracks[track].ai_header + as->game_ctx.tracks[track].ai_header->zones_offset))+i;
            auto z = as->game_ctx.tracks[track].ai_zones[i];
            for (int zone_group = 0; zone_group < 3; zone_group++){
                as->game_ctx.tracks[track].ai_targets[zone_group][i] = ((Object*)((uint8_t*)as->game_ctx.tracks[track].ai_header + as->game_ctx.tracks[track].ai_header->targets_offset))+i;
            }
        }
    }
}