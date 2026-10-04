#pragma once

#include "backend/vulkan_context.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>

namespace xel
{
    class XelModel
    {
        public:

            struct Vertex
            {
                glm::vec2 position;
                glm::vec3 color;

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };

            XelModel(backend::VulkanContext& xelDevice, const std::vector<Vertex> &vertices);
            ~XelModel();

            XelModel(const backend::VulkanContext&) = delete;
            XelModel &operator=(const XelModel&) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);
        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);

            backend::VulkanContext& xelDevice;
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;
    };
}