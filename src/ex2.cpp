#include "backend/vulkan_context.hpp"
#include "backend/render_pass.hpp"
#include "backend/swap_chain.hpp"
#include "backend/ui_renderer.hpp"

int main()
{
    using namespace xel::backend;
    Window window{800, 600, "xel"};
    VulkanContext ctx{window};

    RenderPass renderpass{ctx, SwapChain::query_swapchain_format(ctx)};

    SwapChain* swapchain = new SwapChain{ctx, window.get_extent(), renderpass.handle()};

    UIRenderer renderer{ctx, swapchain, renderpass};

    while (!window.should_close())
    {
        glfwPollEvents();
        renderer.draw_frame();
    }

    vkDeviceWaitIdle(ctx.device());

}