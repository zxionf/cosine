#include "ui_renderer.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstring>

namespace xel::ui
{
    UIRenderer::UIRenderer(backend::Device& device, backend::SwapChain& swapChain)
    : device_(device), swapChain_(swapChain)
    {
    }

    UIRenderer::~UIRenderer()
    {
        vkDeviceWaitIdle(device_.device());
        if (pipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_.device(), pipeline_, nullptr);
        if (pipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_.device(), pipelineLayout_, nullptr);
        if (descriptorSetLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device_.device(), descriptorSetLayout_, nullptr);
        if (descriptorPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(device_.device(), descriptorPool_, nullptr);
        if (vertexBuffer_ != VK_NULL_HANDLE) vkDestroyBuffer(device_.device(), vertexBuffer_, nullptr);
        if (vertexBufferMemory_ != VK_NULL_HANDLE) vkFreeMemory(device_.device(), vertexBufferMemory_, nullptr);
    }

    void UIRenderer::beginFrame()
    {
        commands_.clear();
        currentFrame_ = (currentFrame_ + 1) % backend::SwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void UIRenderer::endFrame()
    {
        // Commands are flushed per-frame during render
    }

    void UIRenderer::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const glm::vec2& uv0, const glm::vec2& uv1)
    {
        UICommand cmd;
        cmd.type = UICommand::Type::Quad;
        cmd.position = position;
        cmd.size = size;
        cmd.color = color;
        cmd.uv0 = uv0;
        cmd.uv1 = uv1;
        commands_.push_back(cmd);
    }

    void UIRenderer::drawText(const glm::vec2& position, const std::string& text, const glm::vec4& color, float fontSize)
    {
        if (!fontAtlas_) return;
        float x = position.x;
        float baseline = position.y + fontSize;
        for (char ch : text)
        {
            const auto& glyph = fontAtlas_->getGlyph(static_cast<char32_t>(ch));
            UICommand cmd;
            cmd.type = UICommand::Type::Text;
            cmd.position = {x + glyph.bearingX, baseline + glyph.bearingY};
            cmd.size = {glyph.width, glyph.height};
            cmd.color = color;
            cmd.uv0 = {glyph.u0, glyph.v0};
            cmd.uv1 = {glyph.u1, glyph.v1};
            commands_.push_back(cmd);
            x += glyph.advance * (fontSize / 16.f);
        }
    }

    void UIRenderer::flush(VkCommandBuffer commandBuffer)
    {
        if (commands_.empty()) return;
        if (pipeline_ == VK_NULL_HANDLE) return;

        rebuildVertexBuffer(device_.device());
        if (vertexBuffer_ == VK_NULL_HANDLE || vertexBufferMemory_ == VK_NULL_HANDLE) return;

        vertices_.resize(commands_.size() * 6);

        for (size_t i = 0; i < commands_.size(); ++i)
        {
            const auto& cmd = commands_[i];
            float x = cmd.position.x;
            float y = cmd.position.y;
            float w = cmd.size.x;
            float h = cmd.size.y;

            Vertex verts[6] = {
                {{x, y}, cmd.uv0, cmd.color},
                {{x + w, y}, {cmd.uv1.x, cmd.uv0.y}, cmd.color},
                {{x + w, y + h}, cmd.uv1, cmd.color},
                {{x, y}, cmd.uv0, cmd.color},
                {{x + w, y + h}, cmd.uv1, cmd.color},
                {{x, y + h}, {cmd.uv0.x, cmd.uv1.y}, cmd.color},
            };
            std::copy(verts, verts + 6, vertices_.begin() + i * 6);
        }

        VkDeviceSize bufferSize = sizeof(Vertex) * vertices_.size();
        void* data;
        vkMapMemory(device_.device(), vertexBufferMemory_, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices_.data(), bufferSize);
        vkUnmapMemory(device_.device(), vertexBufferMemory_);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer_, &offset);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        VkViewport viewport{0, 0, (float)swapChain_.get_swap_chain_extent().width, (float)swapChain_.get_swap_chain_extent().height, 0.0f, 1.0f};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, swapChain_.get_swap_chain_extent()};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        if (fontAtlas_)
        {
            VkDescriptorSet ds = fontAtlas_->getDescriptorSet();
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &ds, 0, nullptr);
        }

        glm::vec2 viewportSize(swapChain_.get_swap_chain_extent().width, swapChain_.get_swap_chain_extent().height);

        for (size_t i = 0; i < commands_.size(); ++i)
        {
            int32_t useTex = (commands_[i].type == UICommand::Type::Text) ? 1 : 0;
            struct { glm::vec2 vp; int32_t ut; } pc;
            pc.vp = viewportSize;
            pc.ut = useTex;
            vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
            vkCmdDraw(commandBuffer, 6, 1, static_cast<uint32_t>(i * 6), 0);
        }

        commands_.clear();
    }

    void UIRenderer::createPipeline(VkRenderPass renderPass)
    {
        if (pipeline_created_) return;
        pipeline_created_ = true;

        VkDescriptorSetLayout fontLayout = fontAtlas_ ? fontAtlas_->getDescriptorSetLayout() : VK_NULL_HANDLE;

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(float) * 2 + sizeof(int32_t);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = fontLayout != VK_NULL_HANDLE ? 1 : 0;
        pipelineLayoutInfo.pSetLayouts = fontLayout != VK_NULL_HANDLE ? &fontLayout : nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushRange;
        vkCreatePipelineLayout(device_.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout_);

        VkShaderModule vertModule;
        VkShaderModule fragModule;

        auto readFile = [](const std::string& filepath) -> std::vector<char>
        {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);
            if (!file.is_open()) return {};
            size_t size = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(size);
            file.seekg(0);
            file.read(buffer.data(), size);
            file.close();
            return buffer;
        };

        auto vertCode = readFile("/home/zxionf/pros/cosine/build/src/shaders/ui_shader.vert.spv");
        auto fragCode = readFile("/home/zxionf/pros/cosine/build/src/shaders/ui_shader.frag.spv");

        if (vertCode.empty() || fragCode.empty())
            throw std::runtime_error("Failed to load UI shader files!");

        VkShaderModuleCreateInfo vertCreateInfo{};
        vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertCreateInfo.codeSize = vertCode.size();
        vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
        if (vkCreateShaderModule(device_.device(), &vertCreateInfo, nullptr, &vertModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create vertex shader module!");

        VkShaderModuleCreateInfo fragCreateInfo{};
        fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragCreateInfo.codeSize = fragCode.size();
        fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
        if (vkCreateShaderModule(device_.device(), &fragCreateInfo, nullptr, &fragModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create fragment shader module!");

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertModule;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragModule;
        stages[1].pName = "main";

        auto bindingDesc = Vertex::getBindingDescriptions();
        auto attrDesc = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDesc.size());
        vertexInput.pVertexBindingDescriptions = bindingDesc.data();
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
        vertexInput.pVertexAttributeDescriptions = attrDesc.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.width = swapChain_.get_swap_chain_extent().width;
        viewport.height = swapChain_.get_swap_chain_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{{0, 0}, swapChain_.get_swap_chain_extent()};

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttach{};
        blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttach.blendEnable = VK_TRUE;
        blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAttach;

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device_.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create UI pipeline!");
        }

        vkDestroyShaderModule(device_.device(), vertModule, nullptr);
        vkDestroyShaderModule(device_.device(), fragModule, nullptr);
    }

    void UIRenderer::rebuildVertexBuffer(VkDevice device)
    {
        if (vertexBuffer_ != VK_NULL_HANDLE) return;

        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = sizeof(Vertex) * 10000;
        bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &bufInfo, nullptr, &vertexBuffer_);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, vertexBuffer_, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = device_.find_memory_type(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory_);
        vkBindBufferMemory(device, vertexBuffer_, vertexBufferMemory_, 0);
    }
}
