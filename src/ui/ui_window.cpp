#include "ui_window.hpp"
#include <iostream>

namespace xel::ui
{
    UIWindow::UIWindow(backend::Window& window, backend::Device& device, backend::SwapChain& swapChain)
    : window_(window), device_(device), swapChain_(swapChain)
    {
    }

    void UIWindow::processEvents()
    {
        glfwPollEvents();
    }

    void UIWindow::setTitle(const std::string& title)
    {
        glfwSetWindowTitle(window_.getGLFWwindow(), title.c_str());
    }

    void UIWindow::mouseButtonCallback(GLFWwindow* w, int button, int action, int mods)
    {
        auto* win = reinterpret_cast<UIWindow*>(glfwGetWindowUserPointer(w));
        if (!win) return;
        UIEvent event;
        event.type = UIEvent::Type::MouseButton;
        event.mouseButton = button;
        event.mousePressed = (action == GLFW_PRESS);
        if (win->onEvent) win->onEvent(event);
    }

    void UIWindow::cursorPosCallback(GLFWwindow* w, double x, double y)
    {
        auto* win = reinterpret_cast<UIWindow*>(glfwGetWindowUserPointer(w));
        if (!win) return;
        UIEvent event;
        event.type = UIEvent::Type::MouseMove;
        event.mouseX = static_cast<float>(x);
        event.mouseY = static_cast<float>(y);
        if (win->onEvent) win->onEvent(event);
    }

    void UIWindow::keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods)
    {
        auto* win = reinterpret_cast<UIWindow*>(glfwGetWindowUserPointer(w));
        if (!win) return;
        UIEvent event;
        event.type = UIEvent::Type::KeyPress;
        event.key = key;
        if (win->onEvent) win->onEvent(event);
    }

    void UIWindow::charCallback(GLFWwindow* w, unsigned int codepoint)
    {
        auto* win = reinterpret_cast<UIWindow*>(glfwGetWindowUserPointer(w));
        if (!win) return;
        UIEvent event;
        event.type = UIEvent::Type::Char;
        event.character = static_cast<char>(codepoint);
        if (win->onEvent) win->onEvent(event);
    }

    void UIWindow::scrollCallback(GLFWwindow* w, double xoff, double yoff)
    {
        auto* win = reinterpret_cast<UIWindow*>(glfwGetWindowUserPointer(w));
        if (!win) return;
        UIEvent event;
        event.type = UIEvent::Type::Scroll;
        event.scrolled = static_cast<int>(yoff);
        if (win->onEvent) win->onEvent(event);
    }
}
