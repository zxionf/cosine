#pragma once

#include "backend/vulkan_context.hpp"
#include "backend/xel_pipeline.hpp"
#include "xel_game_object.hpp"

#include <memory>

namespace xel
{
    class SimpleRenderSystem
    {
    public:

        SimpleRenderSystem(backend::VulkanContext &device, VkRenderPass renderPass);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem &operator=(const SimpleRenderSystem&) = delete;

        void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<XelGameObject> &gameObjects);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        backend::VulkanContext& device;

        std::unique_ptr<backend::XelPipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    };
}