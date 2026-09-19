#pragma once

#include "../backend/xel_pipeline.hpp"

#include <memory>

namespace xel::ui
{

    class ResourceManager
    {
    public:
        ResourceManager(backend::Device& device, VkRenderPass render_pass);
        ~ResourceManager();

        // void render(VkCommandBuffer command_buffer);
    private:
        void create_pipeline_layout();
        void create_pipeline(VkRenderPass render_pass);

        backend::Device& device_;
        std::unique_ptr<backend::XelPipeline> pipeline_;
        VkPipelineLayout pipeline_layout_;
    };
} // namespace xel::ui
