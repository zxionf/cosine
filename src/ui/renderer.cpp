#include "renderer.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace xel::ui
{
    Renderer::Renderer(xel::backend::Device& device, xel::backend::SwapChain& swap_chain)
        : device_{device}, swap_chain_{swap_chain}
    {
        create_pipeline();
        create_buffers();
    }

    Renderer::~Renderer()
    {
        for (size_t i = 0; i < vertex_buffers_.size(); i++)
        {
            vkDestroyBuffer(device_.device(), vertex_buffers_[i], nullptr);
            vkFreeMemory(device_.device(), vertex_buffer_memories_[i], nullptr);
        }

        if (pipeline_ != VK_NULL_HANDLE)
            vkDestroyPipeline(device_.device(), pipeline_, nullptr);

        if (pipeline_layout_ != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(device_.device(), pipeline_layout_, nullptr);
    }

    void Renderer::begin_frame()
    {
        pending_rects_.clear();
        vertices_.clear();
    }

    void Renderer::draw_rect(const Rect& rect)
    {
        pending_rects_.push_back(rect);
    }

    void Renderer::end_frame()
    {
        // 把像素坐标转成 NDC，生成 6 个顶点（两个三角形）
        const VkExtent2D extent = swap_chain_.get_swap_chain_extent();
        const float w = static_cast<float>(extent.width);
        const float h = static_cast<float>(extent.height);

        vertices_.clear();
        vertices_.reserve(pending_rects_.size() * 6);

        for (const auto& rect : pending_rects_)
        {
            // 像素坐标 -> NDC
            float x0 = (rect.x / w) * 2.0f - 1.0f;
            float y0 = (rect.y / h) * 2.0f - 1.0f;
            float x1 = ((rect.x + rect.width) / w) * 2.0f - 1.0f;
            float y1 = ((rect.y + rect.height) / h) * 2.0f - 1.0f;

            Vertex v0{x0, y0, rect.color.r, rect.color.g, rect.color.b, rect.color.a};
            Vertex v1{x1, y0, rect.color.r, rect.color.g, rect.color.b, rect.color.a};
            Vertex v2{x1, y1, rect.color.r, rect.color.g, rect.color.b, rect.color.a};
            Vertex v3{x0, y1, rect.color.r, rect.color.g, rect.color.b, rect.color.a};

            // 三角形 1: v0 v1 v2
            vertices_.push_back(v0);
            vertices_.push_back(v1);
            vertices_.push_back(v2);

            // 三角形 2: v0 v2 v3
            vertices_.push_back(v0);
            vertices_.push_back(v2);
            vertices_.push_back(v3);
        }

        update_vertex_buffer();
    }

    void Renderer::render(VkCommandBuffer command_buffer)
    {
        if (vertices_.empty())
            return;

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffers_[0], &offset);

        vkCmdDraw(command_buffer, static_cast<uint32_t>(vertices_.size()), 1, 0, 0);
    }

    void Renderer::create_pipeline()
    {
        auto vert_code = read_file("shaders/ui_rect.vert.spv");
        auto frag_code = read_file("shaders/ui_rect.frag.spv");

        VkShaderModule vert_module = create_shader_module(vert_code);
        VkShaderModule frag_module = create_shader_module(frag_code);

        VkPipelineShaderStageCreateInfo vert_stage{};
        vert_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_stage.module = vert_module;
        vert_stage.pName = "main";

        VkPipelineShaderStageCreateInfo frag_stage{};
        frag_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_stage.module = frag_module;
        frag_stage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = {vert_stage, frag_stage};

        auto binding = Vertex::binding_description();
        auto attrs = Vertex::attribute_descriptions();

        VkPipelineVertexInputStateCreateInfo vertex_input{};
        vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input.vertexBindingDescriptionCount = 1;
        vertex_input.pVertexBindingDescriptions = &binding;
        vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
        vertex_input.pVertexAttributeDescriptions = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // alpha 混合
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;

        std::array<VkDynamicState, 2> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 0;
        layout_info.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(device_.device(), &layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
            throw std::runtime_error("failed to create ui pipeline layout!");

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = 2;
        pipeline_info.pStages = stages;
        pipeline_info.pVertexInputState = &vertex_input;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = pipeline_layout_;
        pipeline_info.renderPass = swap_chain_.get_render_pass();
        pipeline_info.subpass = 0;

        if (vkCreateGraphicsPipelines(device_.device(), VK_NULL_HANDLE, 1,
                                       &pipeline_info, nullptr, &pipeline_) != VK_SUCCESS)
            throw std::runtime_error("failed to create ui graphics pipeline!");

        vkDestroyShaderModule(device_.device(), vert_module, nullptr);
        vkDestroyShaderModule(device_.device(), frag_module, nullptr);
    }

    void Renderer::create_buffers()
    {
        constexpr VkDeviceSize INITIAL_SIZE = 1024 * 1024;   // 1 MB
        const size_t frame_count = xel::backend::SwapChain::MAX_FRAMES_IN_FLIGHT;

        vertex_buffers_.resize(frame_count);
        vertex_buffer_memories_.resize(frame_count);
        vertex_buffer_mapped_.resize(frame_count);
        vertex_buffer_sizes_.resize(frame_count, INITIAL_SIZE);

        for (size_t i = 0; i < frame_count; i++)
        {
            VkBufferCreateInfo buffer_info{};
            buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size = INITIAL_SIZE;
            buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device_.device(), &buffer_info, nullptr,
                               &vertex_buffers_[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create ui vertex buffer!");

            VkMemoryRequirements mem_req;
            vkGetBufferMemoryRequirements(device_.device(), vertex_buffers_[i], &mem_req);

            VkMemoryAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_req.size;

            // 主机可见 + 主机一致，方便直接映射写入
            VkPhysicalDeviceMemoryProperties mem_props;
            vkGetPhysicalDeviceMemoryProperties(device_.physical_device(), &mem_props);
            uint32_t mem_type = 0;
            for (uint32_t j = 0; j < mem_props.memoryTypeCount; j++)
            {
                if ((mem_req.memoryTypeBits & (1 << j)) &&
                    (mem_props.memoryTypes[j].propertyFlags &
                     (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                {
                    mem_type = j;
                    break;
                }
            }
            alloc_info.memoryTypeIndex = mem_type;

            if (vkAllocateMemory(device_.device(), &alloc_info, nullptr,
                                 &vertex_buffer_memories_[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to allocate ui vertex buffer memory!");

            vkBindBufferMemory(device_.device(), vertex_buffers_[i],
                               vertex_buffer_memories_[i], 0);

            vkMapMemory(device_.device(), vertex_buffer_memories_[i], 0,
                        INITIAL_SIZE, 0, &vertex_buffer_mapped_[i]);
        }
    }

    void Renderer::update_vertex_buffer()
    {
        if (vertices_.empty())
            return;

        const VkDeviceSize data_size = vertices_.size() * sizeof(Vertex);
        if (data_size > vertex_buffer_sizes_[0])
        {
            // 超出初始容量，先报错，后续可扩展为重建 buffer
            throw std::runtime_error("ui vertex buffer overflow, increase INITIAL_SIZE");
        }

        std::memcpy(vertex_buffer_mapped_[0], vertices_.data(), data_size);
    }

    VkShaderModule Renderer::create_shader_module(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule module;
        if (vkCreateShaderModule(device_.device(), &create_info, nullptr, &module) != VK_SUCCESS)
            throw std::runtime_error("failed to create ui shader module!");
        return module;
    }

    std::vector<char> Renderer::read_file(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("failed to open file: " + path);

        size_t size = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        file.close();
        return buffer;
    }
} // namespace xel::ui