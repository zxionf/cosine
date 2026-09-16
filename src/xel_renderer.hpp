#pragma once

#include "xel_device.hpp"
#include "xel_pipeline.hpp"
#include "xel_swap_chain.hpp"
#include "xel_window.hpp"

#include <memory>
#include <cassert>

namespace xel
{
    class XelRenderer
    {
    public:
        XelRenderer(XelWindow &window, XelDevice &device);
        ~XelRenderer();

        XelRenderer(const XelRenderer &) = delete;
        XelRenderer &operator=(const XelRenderer&) = delete;

        VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
        bool isFrameInProgress() const { return isFrameStarted; }

        VkCommandBuffer getCurrentCommandBuffer() const
        {
            assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const
        {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateSwapChain();

        XelWindow& window;
        XelDevice& device;
        std::unique_ptr<XelSwapChain> swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex = 0;
        bool isFrameStarted;
    };
}