#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace xel
{
    class XelWindow
    {
        public:
            XelWindow(int width, int height, std::string name);
            ~XelWindow();

            XelWindow(const XelWindow &) = delete;
            XelWindow &operator=(const XelWindow &) = delete;

            bool shouldClose() { return glfwWindowShouldClose(window); }
            VkExtent2D getExtend() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
            bool wasWindowResized() { return framebufferResized; }
            void resetWindowResizedFlag() { framebufferResized = false; }

            void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
        private:
            static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
            void initWindow();
            int width;
            int height;
            bool framebufferResized = false;

            std::string windowName;
            GLFWwindow* window;
    };
}