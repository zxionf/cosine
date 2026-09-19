#include "res_manager.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>

namespace xel::ui
{
    struct SimplePushConstantData
    {
        // glm::mat2 transform{1.f};
        glm::vec2 offset;
        alignas(16) glm::vec4 color;
    };

    ResourceManager::ResourceManager(backend::Device& device, VkRenderPass render_pass)
    : device_{device}
    {
        create_pipeline_layout();
        create_pipeline(render_pass);
    }

    ResourceManager::~ResourceManager()
    {
        vkDestroyPipelineLayout(device_.device(), pipeline_layout_, nullptr);
    }

    void ResourceManager::create_pipeline_layout()
    {
        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        push_constant_range.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 0;
        pipeline_layout_info.pSetLayouts = nullptr;
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_constant_range;
        if(vkCreatePipelineLayout(device_.device(), &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout");
    }

    void ResourceManager::create_pipeline(VkRenderPass render_pass)
    {
        assert(pipeline_layout_ != nullptr && "Cannot create pipeline before pipeline layout");

        backend::PipelineConfigInfo pipeline_config{};
        backend::XelPipeline::defaultPipelineConfigInfo(pipeline_config);
        pipeline_config.renderPass = render_pass;
        pipeline_config.pipelineLayout = pipeline_layout_;
        pipeline_ = std::make_unique<backend::XelPipeline>(device_, "shaders/rect_ui.vert.spv", "shaders/rect_ui.frag.spv", pipeline_config);
    }
} // namespace xel::ui
