#include "imgui.h"
void imgui_init_config(ImGuiStyle& style)
{
    style.DockingNodeHasCloseButton = false;
    style.FrameBorderSize = 2.0f;

    style.WindowRounding = 8.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.TreeLinesRounding = 8.0f;
}