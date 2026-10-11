#pragma once

#include "backend/window.hpp"
#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
#include "backend/vulkan/ui_renderer.hpp"
#include "ui/tester.hpp"

namespace xel
{
    class Application
    {
    public:
        backend::Window window_;
        backend::vulkan::VulkanContext vulkan_context_;
        backend::vulkan::SwapChain swap_chain_;
        backend::vulkan::UIRenderer renderer_;
        ui::Tester event_tester_;

        Application(int window_w, int window_h, const std::string& title);
        ~Application();
    private:
        static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
        static void cursor_pos_callback(GLFWwindow* window, double mx, double my);
        static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    };
} // namespace xel
