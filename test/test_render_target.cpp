#include "backend/vulkan/render_target.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using xel::backend::vulkan::RenderTarget;

    RenderTarget target;
    assert(!target.valid());
    assert(!target.is_swapchain);
    assert(target.initial_layout == vk::ImageLayout::eUndefined);
    assert(target.final_layout == vk::ImageLayout::ePresentSrcKHR);

    target.extent.width = 1280;
    target.extent.height = 720;
    target.format = vk::Format::eB8G8R8A8Srgb;
    target.initial_layout = vk::ImageLayout::eUndefined;
    target.is_swapchain = true;
    target.image_index = 2;

    assert(target.extent.width == 1280);
    assert(target.extent.height == 720);
    assert(target.format == vk::Format::eB8G8R8A8Srgb);
    assert(target.is_swapchain);
    assert(target.image_index == 2);
    assert(!target.valid()); // 没有真实 image/view，仍不是有效目标

    std::cout << "render target tests passed\n";
    return 0;
}
