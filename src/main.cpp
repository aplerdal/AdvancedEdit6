#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include <iostream>
#include "editor.hpp"
#include "menubar.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_HEIGHT 800
#define WINDOW_WIDTH 1600

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{

    if (!SDL_SetAppMetadata("Advanced Edit", "1.0", "dev.aplerdal.advancededit")) {
        return SDL_APP_FAILURE;
    }

    for (int i = 0; i < SDL_arraysize(extended_metadata); i++) {
        if (!SDL_SetAppMetadataProperty(extended_metadata[i].key, extended_metadata[i].value)) {
            return SDL_APP_FAILURE;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    AppState *as = new AppState;
    *appstate = as;

    as->editor_ctx.active_scenes.push_back(new MenuBar);

    if (!SDL_CreateWindowAndRenderer("AdvancedEdit", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &as->window, &as->renderer)) {
        return SDL_APP_FAILURE;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(as->window, as->renderer);
    ImGui_ImplSDLRenderer3_Init(as->renderer);
    
    as->last_step = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState* as = (AppState*)appstate;
    ImGui_ImplSDL3_ProcessEvent(event);
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(as->window))
        return SDL_APP_SUCCESS;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState* as = (AppState*)appstate;
    // Run slower when minimized
    if (SDL_GetWindowFlags(as->window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10);
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    // Imgui Window Code:
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    ImGui::ShowDemoWindow();
    int j = 0;
    for (Scene* i : as->editor_ctx.active_scenes){
        i->update(as, j++);
    }
    // End window code

    ImGui::Render();
    
    SDL_SetRenderDrawColorFloat(as->renderer, 0, 0, 0, 0);
    SDL_RenderClear(as->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), as->renderer);
    SDL_RenderPresent(as->renderer);

    return as->editor_ctx.app_result;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppState* as = (AppState*)appstate;
    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(as->renderer);
    SDL_DestroyWindow(as->window);
    for (auto o : as->editor_ctx.active_scenes) {
        delete o;
    }
    delete appstate;
    SDL_Quit();
}