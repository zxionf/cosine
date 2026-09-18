#include "backend/device.hpp"
#include "backend/swap_chain.hpp"

#include <memory>

int main()
{
    using namespace xel::backend;
    Window window{800, 600, "Xel"};
    Device device{window};

    std::unique_ptr<SwapChain> swap_chain;
    if (swap_chain == nullptr) swap_chain = std::make_unique<SwapChain>(device, window.get_extent());

    while (!window.should_close())
    {
        glfwPollEvents();

        if (window.was_window_resized())
        {
            std::shared_ptr<SwapChain> old_swap_chain = std::move(swap_chain);
            swap_chain = std::make_unique<SwapChain>(device, window.get_extent(), old_swap_chain);
            window.reset_window_resized_flag();
        }
    }

    // vkDeviceWaitIdle(device.device());
}