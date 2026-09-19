#include "font_atlas.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace xel::ui
{
    namespace
    {
        constexpr int ATLAS_SIZE = 512;
        constexpr int PADDING = 2;
    }

    FontAtlas::~FontAtlas()
    {
        destroy();
    }

    bool FontAtlas::load(const std::string& fontPath, float fontSize, backend::Device& device, VkCommandPool commandPool, VkQueue graphicsQueue)
    {
        device_ = &device;
        fontSize_ = fontSize;

        std::ifstream file(fontPath, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open font file: " << fontPath << std::endl;
            return false;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        fileSize_ = fileSize;
        std::vector<char> fontData(fileSize);
        file.seekg(0);
        file.read(fontData.data(), fileSize);
        file.close();

        stbtt_fontinfo fontInfo;
        if (!stbtt_InitFont(&fontInfo, reinterpret_cast<const unsigned char*>(fontData.data()), 0))
        {
            std::cerr << "Failed to init font" << std::endl;
            return false;
        }

        int ascender, descender, lineGap;
        stbtt_GetFontVMetrics(&fontInfo, &ascender, &descender, &lineGap);
        float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize_);
        lineHeight_ = (ascender - descender) * scale;

        atlasWidth_ = ATLAS_SIZE;
        atlasHeight_ = ATLAS_SIZE;
        pixels_ = new unsigned char[ATLAS_SIZE * ATLAS_SIZE];
        std::memset(pixels_, 0, ATLAS_SIZE * ATLAS_SIZE);

        int x = PADDING;
        int y = PADDING;
        int maxRowHeight = 0;

        for (int ch = 32; ch < 128; ++ch)
        {
            int ix0, iy0, ix1, iy1;
            stbtt_GetCodepointBitmapBox(&fontInfo, ch, scale, scale, &ix0, &iy0, &ix1, &iy1);
            int gw = ix1 - ix0;
            int gh = iy1 - iy0;

            if (x + gw + PADDING * 2 > ATLAS_SIZE)
            {
                x = PADDING;
                y += maxRowHeight + PADDING;
                maxRowHeight = 0;
            }

            if (y + gh + PADDING * 2 > ATLAS_SIZE)
            {
                std::cerr << "Font atlas full, increase ATLAS_SIZE" << std::endl;
                break;
            }

            int advanceWidth, leftSideBearing;
            stbtt_GetCodepointHMetrics(&fontInfo, ch, &advanceWidth, &leftSideBearing);

            Glyph glyph;
            glyph.width = static_cast<float>(gw);
            glyph.height = static_cast<float>(gh);
            glyph.x = static_cast<float>(ix0);
            glyph.y = static_cast<float>(iy0);
            glyph.advance = static_cast<float>(advanceWidth) * scale;
            glyph.bearingX = static_cast<float>(ix0);
            glyph.bearingY = static_cast<float>(iy0);
            glyph.u0 = (x + PADDING) / static_cast<float>(ATLAS_SIZE);
            glyph.v0 = (y + PADDING) / static_cast<float>(ATLAS_SIZE);
            glyph.u1 = (x + PADDING + gw) / static_cast<float>(ATLAS_SIZE);
            glyph.v1 = (y + PADDING + gh) / static_cast<float>(ATLAS_SIZE);
            glyphs_[static_cast<char32_t>(ch)] = glyph;

            unsigned char* bitmap = stbtt_GetCodepointBitmap(&fontInfo, scale, scale, ch, &gw, &gh, nullptr, nullptr);
            if (bitmap)
            {
                for (int row = 0; row < gh; ++row)
                {
                    std::memcpy(pixels_ + (y + PADDING + row) * ATLAS_SIZE + (x + PADDING), bitmap + row * gw, gw);
                }
                stbtt_FreeBitmap(bitmap, nullptr);
            }

            x += gw + PADDING;
            maxRowHeight = std::max(maxRowHeight, gh + PADDING);
        }

        createTexture(commandPool, graphicsQueue);
        createDescriptorSet();
        createPipelineLayout();

        std::cout << "Font atlas loaded: " << fileSize_ << " bytes, " << glyphs_.size() << " glyphs" << std::endl;
        return true;
    }

    void FontAtlas::createTexture(VkCommandPool commandPool, VkQueue graphicsQueue)
    {
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(ATLAS_SIZE * ATLAS_SIZE);
        VkDevice device = device_->device();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = imageSize;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuffer);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = device_->find_memory_type(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);
        vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

        void* data;
        vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data);
        std::memcpy(data, pixels_, imageSize);
        vkUnmapMemory(device, stagingMemory);

        VkImageCreateInfo imgInfo{};
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.extent = {static_cast<uint32_t>(ATLAS_SIZE), static_cast<uint32_t>(ATLAS_SIZE), 1};
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 1;
        imgInfo.format = VK_FORMAT_R8_UNORM;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(device, &imgInfo, nullptr, &image_);
        vkGetImageMemoryRequirements(device, image_, &memReq);
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = device_->find_memory_type(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory_);
        vkBindImageMemory(device, image_, imageMemory_, 0);

        VkCommandBuffer cmd;
        VkCommandBufferAllocateInfo cmdAlloc{};
        cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAlloc.commandPool = commandPool;
        cmdAlloc.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &cmdAlloc, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = image_;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {ATLAS_SIZE, ATLAS_SIZE, 1};
        vkCmdCopyBufferToImage(cmd, stagingBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &cmd);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8_UNORM;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        vkCreateImageView(device, &viewInfo, nullptr, &imageView_);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.maxAnisotropy = 1.0f;
        vkCreateSampler(device, &samplerInfo, nullptr, &sampler_);
    }

    void FontAtlas::createDescriptorSet()
    {
        VkDevice device = device_->device();

        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = 0;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &layoutBinding;
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout_);

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool_;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout_;
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet_);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView_;
        imageInfo.sampler = sampler_;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet_;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    void FontAtlas::createPipelineLayout()
    {
        VkDevice device = device_->device();

        VkDescriptorSetLayout setLayouts[] = {descriptorSetLayout_};
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = setLayouts;
        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout_);
    }

    const Glyph& FontAtlas::getGlyph(char32_t ch) const
    {
        static Glyph empty{};
        auto it = glyphs_.find(ch);
        return it != glyphs_.end() ? it->second : empty;
    }

    void FontAtlas::destroy()
    {
        VkDevice device = device_->device();
        if (imageView_) vkDestroyImageView(device, imageView_, nullptr);
        if (image_) vkDestroyImage(device, image_, nullptr);
        if (imageMemory_) vkFreeMemory(device, imageMemory_, nullptr);
        if (sampler_) vkDestroySampler(device, sampler_, nullptr);
        if (descriptorPool_) vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
        if (descriptorSetLayout_) vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
        if (pipelineLayout_) vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        if (pixels_) { delete[] pixels_; pixels_ = nullptr; }
    }
}
