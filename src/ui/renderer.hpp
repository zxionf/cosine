#pragma once

#include "../backend/device.hpp"
#include "../backend/swap_chain.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>

namespace xel::ui
{
    struct Color
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct Rect
    {
        float x = 0.0f;      // 左上角，像素坐标
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        Color color{};
    };

    class Renderer
    {
    public:
        Renderer(xel::backend::Device& device, xel::backend::SwapChain& swap_chain);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        // 设置当前帧要画的矩形
        void begin_frame();
        void draw_rect(const Rect& rect);
        void end_frame();

        // 把收集到的矩形提交到 command buffer
        void render(VkCommandBuffer command_buffer);

    private:
        struct Vertex
        {
            float x, y;
            float r, g, b, a;

            static VkVertexInputBindingDescription binding_description()
            {
                VkVertexInputBindingDescription desc{};
                desc.binding = 0;
                desc.stride = sizeof(Vertex);
                desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                return desc;
            }

            static std::vector<VkVertexInputAttributeDescription> attribute_descriptions()
            {
                std::vector<VkVertexInputAttributeDescription> attrs(2);
                attrs[0].binding = 0;
                attrs[0].location = 0;
                attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
                attrs[0].offset = offsetof(Vertex, x);

                attrs[1].binding = 0;
                attrs[1].location = 1;
                attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                attrs[1].offset = offsetof(Vertex, r);
                return attrs;
            }
        };

        void create_pipeline();
        void create_buffers();
        void update_vertex_buffer();

        VkShaderModule create_shader_module(const std::vector<char>& code);
        static std::vector<char> read_file(const std::string& path);

        xel::backend::Device& device_;
        xel::backend::SwapChain& swap_chain_;

        VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;

        std::vector<VkBuffer> vertex_buffers_;
        std::vector<VkDeviceMemory> vertex_buffer_memories_;
        std::vector<void*> vertex_buffer_mapped_;
        std::vector<VkDeviceSize> vertex_buffer_sizes_;

        std::vector<Vertex> vertices_;       // CPU 端暂存当前帧顶点
        std::vector<Rect> pending_rects_;    // 当前帧待绘制的矩形
    };
} // namespace xel::ui