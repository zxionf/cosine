#pragma once

#include "vulkan_context.hpp"

namespace xel::backend
{
    class RenderPass
    {
    public:
        RenderPass(VulkanContext& context, VkFormat color_format);
        ~RenderPass();

        VkRenderPass handle() { return render_pass_; }

    private:
        void create_render_pass(VkFormat swap_chain_image_format);

        VkRenderPass render_pass_ = VK_NULL_HANDLE;

        VulkanContext& context_;
    };
} // namespace xel::backend
