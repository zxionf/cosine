#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <map>
#include "backend/device.hpp"

namespace xel::ui
{
    struct Glyph
    {
        float x = 0, y = 0, width = 0, height = 0;
        float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
        float advance = 0;
        float bearingX = 0, bearingY = 0;
    };

    class FontAtlas
    {
    public:
        FontAtlas() = default;
        ~FontAtlas();

        bool load(const std::string& fontPath, float fontSize, backend::Device& device, VkCommandPool commandPool, VkQueue graphicsQueue);
        void destroy();

        const Glyph& getGlyph(char32_t ch) const;
        VkImageView getImageView() const { return imageView_; }
        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout_; }
        VkDescriptorSet getDescriptorSet() const { return descriptorSet_; }
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout_; }
        VkSampler getSampler() const { return sampler_; }
        float getLineHeight() const { return lineHeight_; }

    private:
        void createTexture(VkCommandPool commandPool, VkQueue graphicsQueue);
        void createDescriptorSet();
        void createPipelineLayout();

        backend::Device* device_ = nullptr;
        VkImage image_ = VK_NULL_HANDLE;
        VkDeviceMemory imageMemory_ = VK_NULL_HANDLE;
        VkImageView imageView_ = VK_NULL_HANDLE;
        VkSampler sampler_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;

        std::map<char32_t, Glyph> glyphs_;
        float lineHeight_ = 0.f;
        float fontSize_ = 0.f;
        int atlasWidth_ = 0;
        int atlasHeight_ = 0;
        unsigned char* pixels_ = nullptr;
        size_t fileSize_ = 0;
    };
}
