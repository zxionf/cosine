#pragma once

#include "vulkan_context.hpp"
#include "swap_chain.hpp"

namespace xel::backend::vulkan
{
    class UIRenderer
    {
    public:
        constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

        UIRenderer(VulkanContext& context, SwapChain& swap_chain);
        ~UIRenderer();

        void draw_frame();

    private:
        void create_pipeline();
        void create_command_pool();
        void create_command_buffers();
        void create_sync_objects();

        void record_command_buffer(uint32_t image_index);

        std::vector<char> read_file(const std::string& filename);
        vk::raii::ShaderModule create_shader_module(const std::vector<char>& code) const;
        void transition_image_layout(
            uint32_t                imageIndex,
            vk::ImageLayout         old_layout,
            vk::ImageLayout         new_layout,
            vk::AccessFlags2        src_access_mask,
            vk::AccessFlags2        dst_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dst_stage_mask
        );

        vk::raii::PipelineLayout pipeline_layout_ = nullptr;
        vk::raii::Pipeline       pipeline_        = nullptr;

        vk::raii::CommandPool   command_pool_   = nullptr;
        std::vector<vk::raii::CommandBuffer> command_buffers_;

        std::vector<vk::raii::Semaphore> present_complete_semaphores_;
        std::vector<vk::raii::Semaphore> render_finished_semaphores_;
        std::vector<vk::raii::Fence>     in_flight_fences_;
        uint32_t frame_index_ = 0;

        VulkanContext& context_;
        SwapChain& swap_chain_;
    };
} // namespace xel::backend::vulkan