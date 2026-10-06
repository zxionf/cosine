#pragma once

#include "vulkan_context.hpp"
#include "swap_chain.hpp"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

namespace xel::backend::vulkan
{

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 uv;

        static vk::VertexInputBindingDescription get_binding_description()
        {
            return {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = vk::VertexInputRate::eVertex
            };
        }
        static std::array<vk::VertexInputAttributeDescription, 3> get_attribute_description()
        {
            return {{
                {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
                {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)},
                {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, uv)}
            }};
        }
    };

    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    const float texelU = 0.5f / 208.0f;
    const float texelV = 0.5f / 285.0f;

    const float u0 = texelU;
    const float v0 = texelV;
    const float u1 = 1.0f - texelU;
    const float v1 = 1.0f - texelV;

    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {u0, v0}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {u0, v1}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {u1, v1}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {u1, v0}}
    };

    // 0        3
    //
    //
    // 1        2

    const std::vector<uint16_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    struct ShapeMaker
    {
        // 生成正 N 边形
        // n: 边数，radius: 外接圆半径，rotation_deg: 旋转角度
        static std::vector<Vertex> makePolygonVertices(int n, float radius, float rotation_deg)
        {
            std::vector<Vertex> verts;
            float rot = rotation_deg * 3.14159265f / 180.0f;
            for (int i = 0; i < n; i++) {
                float angle = rot + i * 2.0f * 3.14159265f / n - 3.14159265f / 2.0f;
                float x = std::cos(angle) * radius;
                float y = std::sin(angle) * radius;
                // 颜色按角度渐变
                float t = (float)i / n;
                verts.push_back({{x, y}, {t, 1.0f - t, 0.5f}});
            }
            return verts;
        }

        // 生成心形
        // scale: 缩放，segments: 采样段数（越多越平滑）
        static std::vector<Vertex> makeHeartVertices(float scale, int segments)
        {
            std::vector<Vertex> verts;
            for (int i = 0; i <= segments; i++) {
                float t = (float)i / segments * 2.0f * 3.14159265f;
                // 心形参数方程
                float x = 16.0f * std::pow(std::sin(t), 3.0f);
                float y = 13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t)
                        - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t);
                // 缩放到 [-0.5, 0.5] 范围
                x = x / 16.0f * scale;
                y = -y / 16.0f * scale;  // 翻转 Y，让心形正立

                float r = (x + scale) / (2.0f * scale);
                float g = (y + scale) / (2.0f * scale);
                verts.push_back({{x, y}, {1.0f, 0.2f + g * 0.5f, 0.4f + r * 0.4f}});
            }
            return verts;
        }

        // 生成花瓣图形
        // petals: 花瓣数，r_outer: 外半径，r_inner: 内半径
        static std::vector<Vertex> makeFlowerVertices(int petals, float r_outer, float r_inner, int segments)
        {
            std::vector<Vertex> verts;
            for (int i = 0; i <= segments; i++) {
                float t = (float)i / segments * 2.0f * 3.14159265f;
                // r 随角度做正弦波动，形成花瓣
                float r = r_inner + (r_outer - r_inner) * std::abs(std::sin(t * petals / 2.0f));
                float x = std::cos(t) * r;
                float y = std::sin(t) * r;
                float hue = t / (2.0f * 3.14159265f);
                verts.push_back({{x, y}, {hue, 0.5f + 0.5f * std::sin(t), 1.0f - hue}});
            }
            return verts;
        }

        static std::vector<uint16_t> makeFanIndices(size_t vertex_count)
        {
            std::vector<uint16_t> indices;
            for (size_t i = 1; i + 1 < vertex_count; i++) {
                indices.push_back(0);
                indices.push_back((uint16_t)i);
                indices.push_back((uint16_t)(i + 1));
            }
            return indices;
        }

        // 生成环形
        // r_outer: 外半径，r_inner: 内半径，segments: 段数
        static std::vector<Vertex> makeRingVertices(float r_outer, float r_inner, int segments)
        {
            std::vector<Vertex> verts;
            for (int i = 0; i <= segments; i++) {
                float t = (float)i / segments * 2.0f * 3.14159265f;
                float cx = std::cos(t);
                float cy = std::sin(t);
                float hue = (float)i / segments;

                // 外圈
                verts.push_back({{cx * r_outer, cy * r_outer}, {hue, 0.8f, 1.0f - hue}});
                // 内圈
                verts.push_back({{cx * r_inner, cy * r_inner}, {hue, 0.4f, 1.0f - hue}});
            }
            return verts;
        }

        static std::vector<uint16_t> makeRingIndices(size_t vertex_count)
        {
            std::vector<uint16_t> indices;
            // 每两个顶点（外圈、内圈）构成一个四边形，拆成两个三角形
            for (size_t i = 0; i + 3 < vertex_count; i += 2) {
                indices.push_back((uint16_t)(i));
                indices.push_back((uint16_t)(i + 1));
                indices.push_back((uint16_t)(i + 2));

                indices.push_back((uint16_t)(i + 1));
                indices.push_back((uint16_t)(i + 3));
                indices.push_back((uint16_t)(i + 2));
            }
            return indices;
        }
    };

    class FontRenderer
    {
    public:
        constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

        // std::vector<Vertex> vertices = {};
        // std::vector<uint16_t> indices = {};

        FontRenderer(VulkanContext& context, SwapChain& swap_chain);
        ~FontRenderer();

        void draw_frame();

    private:
        void create_descriptor_set_layout();
        void create_pipeline();
        void create_command_pool();
        void create_texture_image();
        void create_texture_image_view();
        void create_texture_sampler();
        void create_vertex_buffer();
        void create_index_buffer();
        void create_uniform_buffers();
        void create_descriptor_pool();
        void create_descriptor_sets();
        void create_command_buffers();
        void create_sync_objects();

        void update_uniform_buffer(uint32_t current_image);

        void record_command_buffer(uint32_t image_index);

        std::pair<vk::raii::Image, vk::raii::DeviceMemory> create_image(
            uint32_t width,
            uint32_t height,
            vk::Format format,
            vk::ImageTiling tiling,
            vk::ImageUsageFlags usage,
            vk::MemoryPropertyFlags properties
        );

        vk::raii::CommandBuffer begin_single_time_commands();
        void end_single_time_commands(vk::raii::CommandBuffer&& command_buffer);

        vk::raii::ImageView create_image_view(const vk::Image& image, vk::Format format);

        void transition_image_layout(vk::raii::CommandBuffer& command_buffer, const vk::raii::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

        void copy_buffer_to_image(vk::raii::CommandBuffer& command_buffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);

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

        vk::raii::DescriptorSetLayout descriptor_set_layout_ = nullptr;

        vk::raii::PipelineLayout pipeline_layout_ = nullptr;
        vk::raii::Pipeline       pipeline_        = nullptr;

        vk::raii::CommandPool   command_pool_         = nullptr;
        std::vector<vk::raii::CommandBuffer> command_buffers_;
        vk::raii::Buffer        vertex_buffer_        = nullptr;
        vk::raii::DeviceMemory  vertex_buffer_memory_ = nullptr;
        vk::raii::Buffer        index_buffer_         = nullptr;
        vk::raii::DeviceMemory  index_buffer_memory_  = nullptr;

        vk::raii::Image        texture_image_        = nullptr;
        vk::raii::DeviceMemory texture_image_memory_ = nullptr;
        vk::raii::ImageView    texture_image_view_   = nullptr;
        vk::raii::Sampler      texture_sampler_      = nullptr;

        vk::raii::DescriptorPool descriptor_pool_ = nullptr;
        std::vector<vk::raii::DescriptorSet> descriptor_sets_;

        std::vector<vk::raii::Buffer> uniform_buffers_;
        std::vector<vk::raii::DeviceMemory> uniform_buffers_memory_;
        std::vector<void*> uniform_buffers_mapped_;

        std::vector<vk::raii::Semaphore> present_complete_semaphores_;
        std::vector<vk::raii::Semaphore> render_finished_semaphores_;
        std::vector<vk::raii::Fence>     in_flight_fences_;
        uint32_t frame_index_ = 0;

        VulkanContext& context_;
        SwapChain& swap_chain_;
    };
} // namespace xel::backend::vulkan