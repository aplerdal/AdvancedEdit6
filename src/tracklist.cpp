#include "tracklist.hpp"

#include <SDL3/SDL.h>
#include "imgui.h"
#include "gbalzss.hpp"

#include <string>
#include <algorithm>

#include "tileset.hpp"
#include "graphics.hpp"
#include "tilemap.hpp"

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
    while (as->editor_ctx.undo_stack.size() > 0)
    {
        delete as->editor_ctx.undo_stack.front();
        as->editor_ctx.undo_stack.pop_front();
    }
    while (as->editor_ctx.redo_stack.size() > 0)
    {
        delete as->editor_ctx.redo_stack.front();
        as->editor_ctx.redo_stack.pop_front();
    }
    
    Tileset::generate_cache(as, track);
    Tilemap::generate_cache(as, track);
}