#pragma once

#include "xel_device.hpp"

// std
#include <string>
#include <vector>

namespace xel
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
            XelDevice &device,
            const std::string &vertFilepath,
            const std::string &fragFilepath,
            const PipelineConfigInfo &configInfo
        );
        ~XelPipeline();

        XelPipeline(const XelPipeline &) = delete;
        XelPipeline& operator=(const XelPipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer);

        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

    private:
        static std::vector<char> readFile(const std::string &filepath);
        void createGraphicsPipeline(
            const std::string &vertFilepath,
            const std::string &fragFilepath,
            const PipelineConfigInfo &configInfo
        );

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);
        XelDevice &xelDevice;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
}