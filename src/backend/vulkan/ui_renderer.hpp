#pragma once

#include "vulkan_context.hpp"
#include "swap_chain.hpp"
#include "../../graphics/vertex.hpp"
#include "../../graphics/triangle.hpp"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

namespace xel::backend::vulkan
{
    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct PushConstant
    {
        glm::vec2 scale;
        glm::vec2 translate;
        float time;
    };

    using Vertex = graphics::Vertex;
    // const std::vector<Vertex> vertices = {
    //     {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    //     {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    //     {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    //     {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    // };

    // const std::vector<uint16_t> indices = {
    //     0, 1, 2,
    //     2, 3, 0
    // };

    class UIRenderer
    {
    public:
        constexpr static int MAX_FRAMES_IN_FLIGHT = 2;
        enum TextureSlot {WHITE = 0, TEXTURE = 1};

        // std::vector<Vertex> vertices = {};
        // std::vector<uint16_t> indices = {};

        UIRenderer(VulkanContext& context, SwapChain& swap_chain);
        ~UIRenderer();

        void draw_frame();

        void begin_frame();
        void end_frame();

    private:
        void create_descriptor_set_layout();
        void create_pipeline();
        void create_command_pool();
        void create_white_texture();
        void create_texture_image();
        void create_texture_image_view();
        void create_texture_sampler();
        void create_dynamic_buffers();
        // void create_vertex_buffer();
        // void create_index_buffer();
        void create_uniform_buffers();
        void create_descriptor_pool();
        void create_descriptor_sets();
        void create_command_buffers();
        void create_sync_objects();

        void update_uniform_buffer();
        void update_push_constants();

        void upload_vertex_data();

        void begin_rendering(vk::raii::CommandBuffer& command_buffer);
        void end_rendering(vk::raii::CommandBuffer& command_buffer);

        // void record_command_buffer(uint32_t image_index);
        // vk::raii::CommandBuffer& begin_command_buffer();
        // void end_command_buffer(vk::raii::CommandBuffer& command_buffer);
        vk::raii::DescriptorSet& descriptor_set(uint32_t index, TextureSlot slot) { return descriptor_sets_[index * 2 + static_cast<uint32_t>(slot)]; }

        std::pair<vk::raii::Image, vk::raii::DeviceMemory> create_image(uint32_t width, uint32_t height,  vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
        vk::raii::ImageView create_image_view(const vk::Image& image, vk::Format format);
        void copy_buffer_to_image(vk::raii::CommandBuffer& command_buffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);

        vk::raii::CommandBuffer begin_single_time_commands();
        void end_single_time_commands(vk::raii::CommandBuffer&& command_buffer);

        void transition_image_layout(vk::raii::CommandBuffer& command_buffer, const vk::raii::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

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

        void save_swapchain_image_to_png(uint32_t image_index, const std::string& path);

        vk::raii::DescriptorSetLayout descriptor_set_layout_ = nullptr;

        vk::raii::PipelineLayout pipeline_layout_ = nullptr;
        vk::raii::Pipeline       pipeline_        = nullptr;

        vk::raii::CommandPool   command_pool_         = nullptr;
        std::vector<vk::raii::CommandBuffer> command_buffers_;
        // vk::raii::Buffer        vertex_buffer_        = nullptr;
        // vk::raii::DeviceMemory  vertex_buffer_memory_ = nullptr;
        // vk::raii::Buffer        index_buffer_         = nullptr;
        // vk::raii::DeviceMemory  index_buffer_memory_  = nullptr;

        vk::raii::Image        texture_image_        = nullptr;
        vk::raii::DeviceMemory texture_image_memory_ = nullptr;
        vk::raii::ImageView    texture_image_view_   = nullptr;
        vk::raii::Sampler      texture_sampler_      = nullptr;

        vk::raii::Image        white_image_        = nullptr;
        vk::raii::DeviceMemory white_image_memory_ = nullptr;
        vk::raii::ImageView    white_image_view_   = nullptr;

        vk::raii::DescriptorPool descriptor_pool_ = nullptr;
        std::vector<vk::raii::DescriptorSet> descriptor_sets_;

        std::vector<vk::raii::Buffer> uniform_buffers_;
        std::vector<vk::raii::DeviceMemory> uniform_buffers_memory_;
        std::vector<void*> uniform_buffers_mapped_;

        std::vector<vk::raii::Semaphore> present_complete_semaphores_;
        std::vector<vk::raii::Semaphore> render_finished_semaphores_;
        std::vector<vk::raii::Fence>     in_flight_fences_;
        uint32_t frame_index_ = 0;
        uint32_t image_index_ = 0;

        // 顶点和索引
        std::vector<vk::raii::Buffer>       staging_vertex_buffers_;
        std::vector<vk::raii::DeviceMemory> staging_vertex_memories_;
        std::vector<void*>                  staging_vertex_mapped_;

        std::vector<vk::raii::Buffer>       staging_index_buffers_;
        std::vector<vk::raii::DeviceMemory> staging_index_memories_;
        std::vector<void*>                  staging_index_mapped_;

        std::vector<vk::raii::Buffer>       device_vertex_buffers_;
        std::vector<vk::raii::DeviceMemory> device_vertex_memories_;
        std::vector<vk::raii::Buffer>       device_index_buffers_;
        std::vector<vk::raii::DeviceMemory> device_index_memories_;

        std::vector<Vertex>                 vertices_;
        std::vector<uint16_t>               indices_;
        vk::DeviceSize                      current_vertex_buffer_size_ = 0;
        vk::DeviceSize                      current_index_buffer_size_  = 0;
        static constexpr size_t MAX_VERTICES_ = 65536;
        static constexpr size_t MAX_INDICES_ = 65536 * 3 / 2;

        graphics::Triangle triangle_;

        VulkanContext& context_;
        SwapChain& swap_chain_;
    };
} // namespace xel::backend::vulkan