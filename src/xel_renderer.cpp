#include "xel_renderer.hpp"

#include <stdexcept>
#include <cassert>
#include <array>

namespace xel
{
    XelRenderer::XelRenderer(backend::Window& window, backend::Device& device)
    : window{window}, device{device}
    {
        recreateSwapChain();
        createCommandBuffers();
    }

    XelRenderer::~XelRenderer()
    {
        freeCommandBuffers();
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

    VkCommandBuffer XelRenderer::beginFrame()
    {
        assert(!isFrameStarted && "cannot begin new frame while another frame is in progress");

        auto result = swapChain->acquire_next_image(&currentImageIndex);

        if(result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return nullptr;
        }
        if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image");
        }

        isFrameStarted = true;

        auto commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        return commandBuffer;
    }

    void XelRenderer::endFrame()
    {
        assert(isFrameStarted && "cannot end frame while frame is not in progress");

        auto commandBuffer = getCurrentCommandBuffer();
        if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer");
        }

        auto result = swapChain->submit_command_buffers(&commandBuffer, &currentImageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.was_window_resized())
        {
            window.reset_window_resized_flag();
            recreateSwapChain();
            isFrameStarted = false;
            currentFrameIndex = (currentFrameIndex + 1) % backend::SwapChain::MAX_FRAMES_IN_FLIGHT;
            return;
        }
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image");
        }

        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % backend::SwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void XelRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(isFrameStarted && "cannot begin render pass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() && "cannot begin render pass on command buffer from a different frame");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapChain->get_render_pass();
        renderPassInfo.framebuffer = swapChain->get_framebuffer(currentImageIndex);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain->get_swap_chain_extent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChain->get_swap_chain_extent().width);
        viewport.height = static_cast<float>(swapChain->get_swap_chain_extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, swapChain->get_swap_chain_extent()};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void XelRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
    {
        assert(isFrameStarted && "cannot end render pass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() && "cannot end render pass on command buffer from a different frame");

        vkCmdEndRenderPass(commandBuffer);
    }
}