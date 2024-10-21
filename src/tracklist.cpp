#include "tracklist.hpp"
#include "imgui.h"
#include "lzss.hpp"
#include "graphics.hpp"
#include "tileset.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <algorithm>
#include "map.hpp"

std::string TrackList::get_name(){
    return "Track List";
}
static void load_gfx_buffer(AppState* as, int track);
void TrackList::update(AppState* as){
    if (!open) return;
    ImGui::Begin("Track List", &open);
    if (!as->editor_ctx.file_open){
        ImGui::Text("No file opened");
        ImGui::End();
        return;
    }
    ImGui::Text(as->game_ctx.track_table->date);
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
                            load_gfx_buffer(as, trackMapping[track+(cup+page*(cupsCount/pagesCount))*tracksCount/cupsCount]);
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

static void load_gfx_buffer(AppState* as, int track){
    as->editor_ctx.selected_track = track;
    Tileset::generate_cache(as, track);
    Map::generate_cache(as, track);
}