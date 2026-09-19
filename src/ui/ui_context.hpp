#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <functional>
#include "backend/window.hpp"
#include "backend/device.hpp"
#include "backend/swap_chain.hpp"
#include "ui_renderer.hpp"
#include "font/font_atlas.hpp"
#include "ui_window.hpp"
#include "widgets/widget.hpp"

namespace xel::ui
{
    class UIContext
    {
    public:
        UIContext(backend::Window& window, backend::Device& device, backend::SwapChain& swapChain);
        ~UIContext();

        void run(std::function<void(float dt)> renderCallback);
        void shutdown();

        void addWidget(std::shared_ptr<Widget> widget);
        void setFontAtlas(FontAtlas* atlas);
        void dispatchEvent(const struct UIEvent& event);

        void drawUI(VkCommandBuffer cmd);

        std::vector<std::shared_ptr<Widget>>& getWidgets() { return widgets_; }

    private:
        void frameLoop();

        backend::Window& window_;
        backend::Device& device_;
        backend::SwapChain& swapChain_;
        std::unique_ptr<UIWindow> uiWindow_;
        std::unique_ptr<UIRenderer> renderer_;
        std::unique_ptr<FontAtlas> fontAtlas_;
        std::vector<std::shared_ptr<class Widget>> widgets_;
        std::vector<VkCommandBuffer> commandBuffers_;
        bool running_ = false;
        uint32_t currentFrame_ = 0;
    };
}
