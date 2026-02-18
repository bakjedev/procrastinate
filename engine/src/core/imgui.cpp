#include "core/imgui.hpp"

#include "core/window.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"

void im_gui_system::Initialize(const Window* window)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL3_InitForVulkan(window->get());
}

void im_gui_system::ProcessEvent(const SDL_Event* event) { ImGui_ImplSDL3_ProcessEvent(event); }
