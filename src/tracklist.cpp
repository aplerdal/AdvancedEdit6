#include "tracklist.hpp"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <string>

void TrackList::init(AppState* as){

}
void TrackList::update(AppState* as, int id){
    ImGui::PushID(id);
    ImGui::Begin("Track List");
    if (!as->editor_ctx.file.is_open()){
        ImGui::Text("No file opened");
        ImGui::End();
        ImGui::PopID();
        return;
    }
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
                            // Open Track from id here
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
    ImGui::PopID();
}
void TrackList::exit(AppState* as){

}