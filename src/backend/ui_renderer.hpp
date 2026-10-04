#pragma once

#include "swap_chain.hpp"
#include "render_pass.hpp"

namespace xel::backend
{
    class UIRenderer
    {
    public:
        UIRenderer(VulkanContext& context, SwapChain* swap_chain, RenderPass& render_pass);
        ~UIRenderer();

        void draw_frame();

    private:
        void create_pipeline();
        void create_command_pools();
        void create_command_buffers();
        void create_sync_objects();

        void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index);

        std::vector<char> read_file(const std::string& filename);
        VkShaderModule create_shader_module(const std::vector<char>& code);

        VkPipelineLayout pipeline_layout_;
        VkPipeline pipeline_;
        VkCommandPool command_pool_;
        VkCommandBuffer command_buffer_;
        // std::vector<VkCommandPool> command_pools_;

        VkSemaphore image_available_semaphore_;
        VkSemaphore render_finished_semaphore_;
        VkFence in_flight_fence_;

        VulkanContext& context_;
        SwapChain* swap_chain_;
        RenderPass& render_pass_;
    };
} // namespace xel::backend