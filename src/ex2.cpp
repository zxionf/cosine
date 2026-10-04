#include "backend/vulkan/vulkan_context.hpp"
#include "backend/vulkan/swap_chain.hpp"
#include "backend/vulkan/ui_renderer.hpp"



int main()
{
    using namespace xel::backend::vulkan;
    Window window{800, 600, "xel"};
    VulkanContext ctx{window};
    SwapChain swap_chain{ctx, window};
    UIRenderer renderer{ctx, swap_chain};

    while (!window.should_close())
    {
        glfwPollEvents();
        renderer.draw_frame();
    }
    ctx.device().waitIdle();
}