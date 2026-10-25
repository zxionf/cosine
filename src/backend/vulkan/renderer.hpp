#pragma once

#include "vulkan_context.hpp"
#include "swap_chain.hpp"
#include "render_target.hpp"
#include "../../graphics/drawable.hpp"
#include "../../graphics/font_atlas.hpp"
#include "../../graphics/font_face.hpp"
#include "../../graphics/frame_stats.hpp"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include <chrono>

namespace xel::backend::vulkan
{
using Vertex = graphics::Vertex;

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

class Renderer
{
public:
    constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

    Renderer(VulkanContext& context, SwapChain& swap_chain);
    ~Renderer();

    graphics::FontAtlas& font_atlas() { return font_atlas_; }
    const graphics::FontAtlas& font_atlas() const { return font_atlas_; }
    const graphics::FrameStats& frame_stats() const { return frame_stats_; }

    // 窗口背景色（主题 window_bg），下一帧生效。
    void set_clear_color(glm::vec4 color) { clear_color_ = color; }
    glm::vec4 clear_color() const { return clear_color_; }

    void begin();
    void end();

    void draw(const graphics::Drawable& d);
    void set_scissor(float x, float y, float w, float h);
    void reset_scissor();

    void draw_line(float x1, float y1, float x2, float y2, float width, glm::vec4 color);
    void draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, glm::vec4 color);
    void draw_rect(float x, float y, float width, float height, glm::vec4 color);
    void draw_rect_uv(float x, float y, float width, float height, glm::vec4 color, float u0, float v0, float u1, float v1);
    void draw_char(float x, float y, float width, char ch, glm::vec4 color);
    void draw_text(float x, float y, float width, const std::string& text, glm::vec4 color);

    // 大概统计
    uint32_t vertices_count() const { uint32_t ret=0; for(const auto& batch : draw_list_.batches()){ret += batch.vertices.size();} return ret; }
    uint32_t indices_count() const { uint32_t ret=0; for(const auto& batch : draw_list_.batches()){ret += batch.indices.size();} return ret; }

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
    void recreate_render_finished_semaphores();

    void update_uniform_buffer();
    void update_push_constants();

    void upload_vertex_data();

    void begin_frame();
    void end_frame();
    RenderTarget make_swapchain_target();
    void begin_rendering(vk::raii::CommandBuffer& command_buffer, const RenderTarget& target);
    void end_rendering(vk::raii::CommandBuffer& command_buffer, const RenderTarget& target);

    vk::raii::DescriptorSet& descriptor_set(uint32_t index, graphics::TextureSlot slot) { return descriptor_sets_[index * 2 + static_cast<uint32_t>(slot)]; }

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
        const RenderTarget&     target,
        vk::ImageLayout         old_layout,
        vk::ImageLayout         new_layout,
        vk::AccessFlags2        src_access_mask,
        vk::AccessFlags2        dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    );

    graphics::DrawList   draw_list_;
    graphics::FontAtlas font_atlas_;
    graphics::FontFace   font_face_;
    uint64_t             font_atlas_revision_ = 0;
    RenderTarget         active_target_{};
    graphics::FrameStats frame_stats_{};
    glm::vec4             clear_color_{0.0f, 0.0f, 0.0f, 0.0f};
    std::chrono::steady_clock::time_point last_frame_time_{};
    bool                 has_last_frame_ = false;
    void update_font_atlas_texture();



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
    bool     frame_acquired_ = false;

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

    static constexpr size_t MAX_VERTICES_ = 65536 * 4;
    static constexpr size_t MAX_INDICES_ = 65536 * 4 * 3 / 2;

    VulkanContext& context_;
    SwapChain& swap_chain_;
};
} // namespace xel::backend::vulkan