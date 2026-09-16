#include "backend/device.hpp"

int main()
{
    using namespace xel::backend;
    Window window{800, 600, "Xel"};
    Device device{window};

    while (!window.should_close())
    {
        glfwPollEvents();
    }

    // vkDeviceWaitIdle(device.device());
}