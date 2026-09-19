#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include "ui/vertex.hpp"
#include "backend/device.hpp"
#include "backend/swap_chain.hpp"
#include "font/font_atlas.hpp"

namespace xel::ui
{
    struct UICommand
    {
        enum class Type : uint8_t
        {
            Quad,
            Text
        };
        Type type = Type::Quad;
        glm::vec2 position;
        glm::vec2 size;
        glm::vec4 color;
        glm::vec2 uv0;
        glm::vec2 uv1;
    };

    class UIRenderer
    {
    public:
        UIRenderer(backend::Device& device, backend::SwapChain& swapChain);
        ~UIRenderer();

        void beginFrame();
        void endFrame();
        void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const glm::vec2& uv0 = {0,0}, const glm::vec2& uv1 = {1,1});
        void drawText(const glm::vec2& position, const std::string& text, const glm::vec4& color, float fontSize = 16.f);
        void flush(VkCommandBuffer commandBuffer);

        void setFontAtlas(FontAtlas* atlas) { fontAtlas_ = atlas; }
        void createPipeline(VkRenderPass renderPass);

    private:
        void rebuildVertexBuffer(VkDevice device);

        bool pipeline_created_ = false;

        backend::Device& device_;
        backend::SwapChain& swapChain_;
        std::vector<UICommand> commands_;
        std::vector<Vertex> vertices_;

        VkPipeline pipeline_ = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
        VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
        VkCommandBuffer currentCommandBuffer_ = VK_NULL_HANDLE;

        FontAtlas* fontAtlas_ = nullptr;
        uint32_t currentFrame_ = 0;
    };
}
