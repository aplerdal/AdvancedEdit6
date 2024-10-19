#include "tracklist.hpp"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <string>

std::string TrackList::get_name(){
    return "Track List";
}

void TrackList::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Track List", &open);
    if (as->editor_ctx.file.size() == 0){
        ImGui::Text("No file opened");
        ImGui::End();
        return;
    }
    ImGui::Text(as->game_ctx.trackTable->date);
    int i = 0;
    for (int page = 0; page < pagesCount; page++){
        ImGui::PushID(page);
        if (ImGui::TreeNode(pagesList[page])){
            for (int cup = 0; cup < cupsCount/pagesCount; cup++){
                ImGui::PushID(cup);
                if (ImGui::TreeNode(cupsList[cup+page*(cupsCount/pagesCount)])){
                    for (int track = 0; track < tracksCount/cupsCount; track++){
                        ImGui::PushID(track);
                        if (ImGui::Button(tracksList[track+(cup+page*(cupsCount/pagesCount))*tracksCount/cupsCount])) {
                            // Load specific track.
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::End();
}