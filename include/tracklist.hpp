#pragma once
#include "editor.hpp"
#include "imgui.h"
#include <SDL3/SDL.h>
#include "types.h"

#define pagesCount 2
#define cupsCount 12
#define tracksCount 48

static const char* pagesList[] = {
    "SNES Tracks",
    "MKSC Tracks"
};
static const char* cupsList[] = {
// SNES
    "Mushroom Cup",
    "Flower Cup",
    "Lightning Cup",
    "Star Cup",
    "Special Cup",
    "Battle",
// MKSC
    "Mushroom Cup",
    "Flower Cup",
    "Lightning Cup",
    "Star Cup",
    "Special Cup",
    "Battle",
};
static const char* tracksList[] = {
// SNES Mushroom
    "Mario Circuit 1",
    "Donut Plains 1",
    "Ghost Valley 1",
    "Bowser Castle 1",
// SNES Flower
    "Mario Circuit 2",
    "Choco Island 1",
    "Ghost Valley 2",
    "Donut Plains 2",
// SNES Lightning
    "Bowser Castle 2",
    "Mario Circuit 3",
    "Koopa Beach 1",
    "Choco Island 2",
// SNES Star
    "Vanilla Lake 1",
    "Bowser Castle 3",
    "Mario Circuit 4",
    "Donut Plains 3",
// SNES Special
    "Koopa Beach 2",
    "Ghost Valley 3",
    "Vanilla Lake 2",
    "Rainbow Road",
// SNES Battle
    "Battle Course 1",
    "Battle Course 2",
    "Battle Course 3",
    "Battle Course 4",
// Mushroom Cup
    "Peach Circuit",
    "Shy Guy Beach",
    "Riverside Park",
    "Bowser Castle 1",
// Flower
    "Mario Circuit",
    "Boo Lake",
    "Cheese Lake",
    "Bowser Castle 2",
// Lightning
    "Luigi Circuit",
    "Sky Garden",
    "Cheep Cheep Island",
    "Sunset Wilds",
// Star
    "Snow Land",
    "Ribbon Road",
    "Yoshi Desert",
    "Bowser Castle 3",
// Special
    "Lakeside Park",
    "Broken Pier",
    "Bowser Castle 4",
    "Rainbow Road",
// Battle
    "Battle Course 1",
    "Battle Course 2",
    "Battle Course 3",
    "Battle Course 4",
};

class TrackList : public Scene {
public:
    void update(AppState* as) override;
    std::string get_name() override;
private:
    int _selected_track;
};