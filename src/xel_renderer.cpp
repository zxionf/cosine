#include "xel_renderer.hpp"
#include <stdexcept>
#include <cassert>
#include <array>

namespace xel
{
    XelRenderer::XelRenderer(backend::Window &window, backend::Device &device)
    : window{window}, device{device}
    {
        recreateSwapChain();
        createCommandBuffers();
        uiContext = std::make_unique<ui::UIContext>(window, device, *swapChain);
    }

    XelRenderer::~XelRenderer()
    {
        freeCommandBuffers();
    }

    void XelRenderer::run()
    {
        uiContext->run([](float dt) {});
    }

    void XelRenderer::createCommandBuffers()
    {
        commandBuffers.resize(backend::SwapChain::MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = device.get_command_pool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if(vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers");
        }
    }

    void XelRenderer::freeCommandBuffers()
    {
        vkFreeCommandBuffers(device.device(), device.get_command_pool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
        commandBuffers.clear();
    }

    void XelRenderer::recreateSwapChain()
    {
        auto extent = window.get_extent();
        while (extent.width == 0 || extent.height == 0)
        {
            extent = window.get_extent();
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device.device());

        if(swapChain == nullptr)
        {
            swapChain = std::make_unique<backend::SwapChain>(device, extent);
        }
        else
        {
            std::shared_ptr<backend::SwapChain> oldSwapChain = std::move(swapChain);
            swapChain = std::make_unique<backend::SwapChain>(device, extent, oldSwapChain);

            if (!oldSwapChain->compare_swap_formats(*swapChain.get()))
            {
                throw std::runtime_error("swap chain image (or depth) format has changed");
            }
        }
    }
}
