#include "ui_context.hpp"
#include "ui_window.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace xel::ui
{
    static UIContext* s_instance = nullptr;

    static void glfwMouseButtonCB(GLFWwindow* w, int button, int action, int mods)
    {
        (void)mods;
        if (!s_instance) return;
        UIEvent e{};
        e.type = UIEvent::Type::MouseButton;
        e.mouseButton = button;
        e.mousePressed = (action == GLFW_PRESS);
        double mx, my;
        glfwGetCursorPos(w, &mx, &my);
        e.mouseX = static_cast<float>(mx);
        e.mouseY = static_cast<float>(my);
        for (auto& widget : s_instance->getWidgets())
        {
            if (widget && widget->isVisible() && widget->isEnabled() && widget->onEvent(e))
                break;
        }
    }

    static void glfwCursorPosCB(GLFWwindow* w, double x, double y)
    {
        (void)w;
        if (!s_instance) return;
        UIEvent e{};
        e.type = UIEvent::Type::MouseMove;
        e.mouseX = static_cast<float>(x);
        e.mouseY = static_cast<float>(y);
        for (auto& widget : s_instance->getWidgets())
        {
            if (widget && widget->isVisible() && widget->isEnabled())
                widget->onEvent(e);
        }
    }

    static void glfwKeyCB(GLFWwindow* w, int key, int scancode, int action, int mods)
    {
        (void)w; (void)scancode; (void)mods;
        if (!s_instance) return;
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        UIEvent e{};
        e.type = UIEvent::Type::KeyPress;
        e.key = key;
        for (auto& widget : s_instance->getWidgets())
        {
            if (widget && widget->isVisible() && widget->isEnabled() && widget->onEvent(e))
                break;
        }
    }

    static void glfwCharCB(GLFWwindow* w, unsigned int codepoint)
    {
        (void)w;
        if (!s_instance) return;
        UIEvent e{};
        e.type = UIEvent::Type::Char;
        e.character = static_cast<char>(codepoint);
        for (auto& widget : s_instance->getWidgets())
        {
            if (widget && widget->isVisible() && widget->isEnabled() && widget->onEvent(e))
                break;
        }
    }

    static void glfwScrollCB(GLFWwindow* w, double xoff, double yoff)
    {
        (void)w; (void)xoff;
        if (!s_instance) return;
        UIEvent e{};
        e.type = UIEvent::Type::Scroll;
        e.scrolled = static_cast<int>(yoff);
        for (auto& widget : s_instance->getWidgets())
        {
            if (widget && widget->isVisible() && widget->isEnabled())
                widget->onEvent(e);
        }
    }

    UIContext::UIContext(backend::Window& window, backend::Device& device, backend::SwapChain& swapChain)
    : window_(window), device_(device), swapChain_(swapChain)
    {
        uiWindow_ = std::make_unique<UIWindow>(window, device, swapChain);
        renderer_ = std::make_unique<UIRenderer>(device, swapChain);
        fontAtlas_ = std::make_unique<FontAtlas>();
        if (!fontAtlas_->load("fonts/DejaVuSans.ttf", 16.f, device, device.get_command_pool(), device.graphics_queue()))
        {
            std::cerr << "Warning: Failed to load font, text rendering disabled" << std::endl;
        }
        renderer_->setFontAtlas(fontAtlas_.get());
    }

    UIContext::~UIContext()
    {
        shutdown();
    }

    void UIContext::run(std::function<void(float dt)> renderCallback)
    {
        s_instance = this;

        GLFWwindow* glfwWin = window_.getGLFWwindow();
        glfwSetMouseButtonCallback(glfwWin, glfwMouseButtonCB);
        glfwSetCursorPosCallback(glfwWin, glfwCursorPosCB);
        glfwSetKeyCallback(glfwWin, glfwKeyCB);
        glfwSetCharCallback(glfwWin, glfwCharCB);
        glfwSetScrollCallback(glfwWin, glfwScrollCB);

        renderer_->createPipeline(swapChain_.get_render_pass());

        commandBuffers_.resize(backend::SwapChain::MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = device_.get_command_pool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());
        if (vkAllocateCommandBuffers(device_.device(), &allocInfo, commandBuffers_.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        currentFrame_ = 0;

        while (!window_.should_close())
        {
            float dt = 0.016f;
            if (renderCallback) renderCallback(dt);
            uiWindow_->processEvents();

            auto extent = window_.get_extent();
            if (extent.width == 0 || extent.height == 0)
            {
                continue;
            }

            uint32_t imageIndex;
            auto result = swapChain_.acquire_next_image(&imageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                continue;
            }
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to acquire swap chain image!");
            }

            VkCommandBuffer cmd = commandBuffers_[currentFrame_];
            vkResetCommandBuffer(cmd, 0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cmd, &beginInfo);

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = swapChain_.get_render_pass();
            renderPassInfo.framebuffer = swapChain_.get_framebuffer(imageIndex);
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChain_.get_swap_chain_extent();
            VkClearValue clearValues[2] = {};
            clearValues[0].color = {{0.3f, 0.3f, 0.3f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = 2;
            renderPassInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            drawUI(cmd);

            vkCmdEndRenderPass(cmd);

            vkEndCommandBuffer(cmd);

            result = swapChain_.submit_command_buffers(&cmd, &imageIndex);
            currentFrame_ = (currentFrame_ + 1) % backend::SwapChain::MAX_FRAMES_IN_FLIGHT;
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                continue;
            }
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to submit draw command buffer!");
            }
        }
    }

    void UIContext::shutdown()
    {
        vkDeviceWaitIdle(device_.device());
        if (!commandBuffers_.empty())
        {
            vkFreeCommandBuffers(device_.device(), device_.get_command_pool(),
                static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
            commandBuffers_.clear();
        }
        widgets_.clear();
    }

    void UIContext::addWidget(std::shared_ptr<Widget> widget)
    {
        widgets_.push_back(widget);
    }

    void UIContext::setFontAtlas(FontAtlas* atlas)
    {
        fontAtlas_.reset(atlas);
        renderer_->setFontAtlas(atlas);
    }

    void UIContext::drawUI(VkCommandBuffer cmd)
    {
        renderer_->beginFrame();
        auto extent = window_.get_extent();
        glm::vec4 bg(0.3f, 0.3f, 0.3f, 1.0f);
        renderer_->drawQuad({0, 0}, {(float)extent.width, (float)extent.height}, bg);
        for (auto& w : widgets_)
        {
            if (w && w->isVisible())
            {
                w->render(*renderer_);
            }
        }
        renderer_->flush(cmd);
    }
}