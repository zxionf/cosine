#pragma once

#include "backend/device.hpp"
#include "backend/window.hpp"
#include "backend/swap_chain.hpp"
#include "ui/ui_context.hpp"

#include <memory>
#include <cassert>

namespace xel
{
    class XelRenderer
    {
    public:
        XelRenderer(backend::Window &window, backend::Device &device);
        ~XelRenderer();

        void run();

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateSwapChain();

        backend::Window& window;
        backend::Device& device;
        std::unique_ptr<backend::SwapChain> swapChain;
        std::unique_ptr<ui::UIContext> uiContext;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex = 0;
        bool isFrameStarted;
    };
}
