#include "backend/window.hpp"
#include "backend/device.hpp"
#include "backend/swap_chain.hpp"

#include <iostream>
#include <array>
#include <stdexcept>

int main()
{
    try
    {
        xel::backend::Window window{800, 600, "Minimal Vulkan Test"};
        xel::backend::Device device{window};
        auto swapChain = std::make_unique<xel::backend::SwapChain>(device, window.get_extent());

        std::vector<VkCommandBuffer> commandBuffers;
        uint32_t currentImageIndex = 0;

        commandBuffers.resize(swapChain->image_count());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = device.get_command_pool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers!");

        std::cerr << "Swapchain extent: " << swapChain->get_swap_chain_extent().width
                  << "x" << swapChain->get_swap_chain_extent().height << std::endl;
        std::cerr << "Image count: " << swapChain->image_count() << std::endl;

        int frameCount = 0;
        while (!window.should_close())
        {
            glfwPollEvents();

            auto extent = window.get_extent();
            if (extent.width == 0 || extent.height == 0)
                continue;

            auto result = swapChain->acquire_next_image(&currentImageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                vkDeviceWaitIdle(device.device());
                swapChain = std::make_unique<xel::backend::SwapChain>(device, window.get_extent(), std::shared_ptr<xel::backend::SwapChain>(std::move(swapChain)));
                commandBuffers.resize(swapChain->image_count());
                VkCommandBufferAllocateInfo ai{};
                ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                ai.commandPool = device.get_command_pool();
                ai.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
                vkAllocateCommandBuffers(device.device(), &ai, commandBuffers.data());
                continue;
            }
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                throw std::runtime_error("failed to acquire swap chain image!");

            VkCommandBuffer cmd = commandBuffers[currentImageIndex];
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
                throw std::runtime_error("failed to begin command buffer!");

            VkRenderPassBeginInfo rpInfo{};
            rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpInfo.renderPass = swapChain->get_render_pass();
            rpInfo.framebuffer = swapChain->get_framebuffer(currentImageIndex);
            rpInfo.renderArea.offset = {0, 0};
            rpInfo.renderArea.extent = swapChain->get_swap_chain_extent();

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{1.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};
            rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            rpInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChain->get_swap_chain_extent().width);
            viewport.height = static_cast<float>(swapChain->get_swap_chain_extent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            VkRect2D scissor{{0, 0}, swapChain->get_swap_chain_extent()};
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            vkCmdEndRenderPass(cmd);

            if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
                throw std::runtime_error("failed to end command buffer!");

            auto submitResult = swapChain->submit_command_buffers(&cmd, &currentImageIndex);
            if (submitResult == VK_ERROR_OUT_OF_DATE_KHR || submitResult == VK_SUBOPTIMAL_KHR)
            {
                vkDeviceWaitIdle(device.device());
                swapChain = std::make_unique<xel::backend::SwapChain>(device, window.get_extent(), std::shared_ptr<xel::backend::SwapChain>(std::move(swapChain)));
                commandBuffers.resize(swapChain->image_count());
                VkCommandBufferAllocateInfo ai{};
                ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                ai.commandPool = device.get_command_pool();
                ai.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
                vkAllocateCommandBuffers(device.device(), &ai, commandBuffers.data());
                continue;
            }
            if (submitResult != VK_SUCCESS)
                throw std::runtime_error("failed to present!");

            if (++frameCount <= 3)
                std::cerr << "Frame " << frameCount << " presented" << std::endl;
        }

        vkDeviceWaitIdle(device.device());
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
