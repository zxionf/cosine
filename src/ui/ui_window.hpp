#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <functional>
#include <GLFW/glfw3.h>
#include "backend/window.hpp"
#include "backend/device.hpp"
#include "backend/swap_chain.hpp"
#include "ui/vertex.hpp"
#include "font/font_atlas.hpp"

namespace xel::ui
{
    struct UIEvent
    {
        enum class Type : uint8_t
        {
            MouseMove,
            MouseButton,
            KeyPress,
            Scroll,
            Resize,
            Char
        };
        Type type = Type::MouseMove;
        float mouseX = 0;
        float mouseY = 0;
        int mouseButton = 0;
        bool mousePressed = false;
        int key = 0;
        int scrolled = 0;
        char character = 0;
    };

    class UIWindow
    {
    public:
        UIWindow(backend::Window& window, backend::Device& device, backend::SwapChain& swapChain);
        ~UIWindow() = default;

        void processEvents();
        bool shouldClose() { return window_.should_close(); }
        void setTitle(const std::string& title);
        backend::Window& getWindow() { return window_; }

        std::function<void(UIEvent&)> onEvent;

    private:
        static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods);
        static void cursorPosCallback(GLFWwindow* w, double x, double y);
        static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
        static void charCallback(GLFWwindow* w, unsigned int codepoint);
        static void scrollCallback(GLFWwindow* w, double xoff, double yoff);

        backend::Window& window_;
        backend::Device& device_;
        backend::SwapChain& swapChain_;
    };
}
