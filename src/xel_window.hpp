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
            void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
        private:
            void initWindow();
            const int width;
            const int height;

            std::string windowName;
            GLFWwindow* window;
    };
}