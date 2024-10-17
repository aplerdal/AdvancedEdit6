#include <SDL3/SDL.h>
#include <fstream>
#include "imgui.h"
#include "editor.hpp"
#include "menubar.hpp"
#include "tracklist.hpp"

void MenuBar::init(AppState* as){

}
void MenuBar::update(AppState* as, int id){
    ImGui::PushID(id);
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open ROM", "ctrl+o")) {
                SDL_ShowOpenFileDialog(
                    OpenFileCallback, 
                    &(as->editor_ctx.file), 
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
            if (ImGui::MenuItem("Track List")) as->editor_ctx.active_scenes.push_back(new TrackList);
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    ImGui::PopID();
}
void MenuBar::exit(AppState* as){
    return;
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

    *((std::ifstream*)userdata) = std::ifstream(*filelist, std::ios::binary);
}