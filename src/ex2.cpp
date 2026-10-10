#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
// #include "backend/vulkan/font_renderer.hpp"
#include "backend/vulkan/ui_renderer.hpp"

#include <iostream>

int main()
{
    using namespace xel::backend::vulkan;
    Window window{400, 400, "xel"};
    VulkanContext ctx{window};
    SwapChain swap_chain{ctx, window};
    // FontRenderer renderer{ctx, swap_chain};
    UIRenderer renderer{ctx, swap_chain};

    while (!window.should_close())
    {
        glfwPollEvents();
        renderer.draw_frame();
    }
    ctx.device().waitIdle();

    std::cout << __FILE__ << std::endl;
}