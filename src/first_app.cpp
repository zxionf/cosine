#include "first_app.hpp"

namespace xel
{
    void FirstApp::run()
    {
        while (!window.shouldClose())
        {
            glfwPollEvents();
        }
    }
}