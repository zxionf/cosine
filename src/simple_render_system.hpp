#pragma once

#include "xel_device.hpp"
#include "xel_pipeline.hpp"
#include "xel_game_object.hpp"

#include <memory>

namespace xel
{
    class SimpleRenderSystem
    {
    public:

        SimpleRenderSystem(XelDevice &device, VkRenderPass renderPass);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem &operator=(const SimpleRenderSystem&) = delete;

        void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<XelGameObject> &gameObjects);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        XelDevice& device;

        std::unique_ptr<XelPipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    };
}