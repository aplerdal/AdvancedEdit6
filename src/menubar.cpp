#include <SDL3/SDL.h>
#include <fstream>
#include "imgui.h"
#include "editor.hpp"
#include "menubar.hpp"
#include "tracklist.hpp"
#include "types.h"

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
                    1, 
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
    if (pos == 0){
        as->editor_ctx.file = std::vector<uint8_t>{};
        return;
    }
    std::vector<uint8_t> buf(pos);
    ifs.seekg(0, std::ios::beg);
    ifs.read((char*)&buf[0], pos);

    as->editor_ctx.file = buf;
    #pragma message ("WARNING: No checks to validate input file. Can result in memory corruption on invalid files.")
    as->editor_ctx.file_open = true;

    // Load Track pointers
    as->game_ctx.track_table = (TrackTable*)(&as->editor_ctx.file.data()[TRACK_TABLE_ADDRESS]);

    as->game_ctx.eof = as->editor_ctx.file.data() + as->editor_ctx.file.size();
    for (int i = 0; i < TRACK_COUNT; i++) {
        as->game_ctx.track_headers[i] = (TrackHeader*)((uint8_t*)as->game_ctx.track_table + as->game_ctx.track_table->track_offsets[i]);
        as->game_ctx.definition_table[i] = (TrackDefinition*)(&as->editor_ctx.file.data()[DEFINITION_TABLE_ADDRESS+i*sizeof(TrackDefinition)]);
    }
}