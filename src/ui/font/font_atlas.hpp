#pragma once

namespace xel::font
{
    struct Glyph
    {
        float x = 0, y = 0, width = 0, height = 0;
        float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
        float advance = 0, bearingX = 0, bearingY = 0;
    };

    class FontAtlas
    {
    public:
        FontAtlas() = default;
        ~FontAtlas();

        bool load(const std::string& font_path, float font_size, backend::Device& device, VkCommandPool command_pool, VkQueue queue);
        void destroy();

        const Glyph& get_glyph(char32_t codepoint) const;
        VkImageView image_view() const { return image_view_; }
        VkDescriptorSetLayout get_descriptor_set_layout() const { return descriptor_set_layout_; }
        VkDescriptorSet get_descriptor_set() const { return descriptor_set_; }
        VkPipelineLayout get_pipeline_layout() const { return pipeline_layout_; }
        VkSampler get_sampler() const { return sampler_; }
        float get_line_height() const { return line_height_; }

    private:
        void create_texture(VkCommandPool command_pool, VkQueue queue);
        void create_descriptor_set();
        void create_pipeline_layout();

        backend::Device& device_;
        VkImage image_ = VK_NULL_HANDLE;
        VkImageView image_view_ = VK_NULL_HANDLE;
        VkDeviceMemory image_memory_ = VK_NULL_HANDLE;
        VkSampler sampler_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;

        std::map<char32_t, Glyph> glyphs_;
        float line_height_ = 0.f;
        float font_size_ = 0.f;
        float atlas_width_ = 0.f;
        float atlas_height_ = 0.f;
        unsigned char* pixels_ = nullptr;
        size_t file_size_ = 0;
    };
} // namespace xel::font