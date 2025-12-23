#include "core/imgui.hpp"
#include "imgui.h"
#include "core/window.hpp"
#include "imgui_impl_sdl3.h"

void ImGuiSystem::initialize(Window* window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL3_InitForVulkan(window->get());
}

void ImGuiSystem::processEvent(const SDL_Event* event) {
  ImGui_ImplSDL3_ProcessEvent(event);
}
