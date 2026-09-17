#pragma once

#include "device.hpp"

#include <string>
#include <vector>

namespace xel::backend
{
    struct PipelineConfigInfo
    {
        PipelineConfigInfo() = default;
        PipelineConfigInfo(const PipelineConfigInfo &) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };
    class XelPipeline
    {
    public:
        XelPipeline(
            Device &device,
            const std::string &vert_filepath,
            const std::string &frag_filepath,
            const PipelineConfigInfo &config_info
        );
        ~XelPipeline();

        XelPipeline(const XelPipeline &) = delete;
        XelPipeline& operator=(const XelPipeline&) = delete;

        void bind(VkCommandBuffer command_buffer);

        static void defaultPipelineConfigInfo(PipelineConfigInfo& config_info);

    private:
        static std::vector<char> readFile(const std::string &filepath);
        void createGraphicsPipeline(
            const std::string &vert_filepath,
            const std::string &frag_filepath,
            const PipelineConfigInfo &config_info
        );

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shader_module);
        Device &device_;
        VkPipeline graphics_pipeline_;
        VkShaderModule vert_shader_module_;
        VkShaderModule frag_shader_module_;
    };
}