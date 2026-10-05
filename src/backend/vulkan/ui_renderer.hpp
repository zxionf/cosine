#pragma once

#include "vulkan_context.hpp"
#include "swap_chain.hpp"

#include <glm/glm.hpp>

namespace xel::backend::vulkan
{

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;

        static vk::VertexInputBindingDescription get_binding_description()
        {
            return {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = vk::VertexInputRate::eVertex
            };
        }
        static std::array<vk::VertexInputAttributeDescription, 2> get_attribute_description()
        {
            return {{{.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
               {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}}};
        }
    };

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

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
        void create_vertex_buffer();
        void create_index_buffer();
        void create_command_buffers();
        void create_sync_objects();

        void record_command_buffer(uint32_t image_index);

        std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void copy_buffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size);

        uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties);

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

        vk::raii::CommandPool   command_pool_         = nullptr;
        vk::raii::Buffer        vertex_buffer_        = nullptr;
        vk::raii::DeviceMemory  vertex_buffer_memory_ = nullptr;
        vk::raii::Buffer        index_buffer_         = nullptr;
        vk::raii::DeviceMemory  index_buffer_memory_  = nullptr;
        std::vector<vk::raii::CommandBuffer> command_buffers_;

        std::vector<vk::raii::Semaphore> present_complete_semaphores_;
        std::vector<vk::raii::Semaphore> render_finished_semaphores_;
        std::vector<vk::raii::Fence>     in_flight_fences_;
        uint32_t frame_index_ = 0;

        VulkanContext& context_;
        SwapChain& swap_chain_;
    };
} // namespace xel::backend::vulkan