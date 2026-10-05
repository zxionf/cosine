#include "ui_renderer.hpp"

#include <fstream>

namespace xel::backend::vulkan
{
    UIRenderer::UIRenderer(VulkanContext& context, SwapChain& swap_chain) : context_{context}, swap_chain_{swap_chain}
    {
        create_pipeline();
        create_command_pool();
        create_vertex_buffer();
        create_index_buffer();
        create_command_buffers();
        create_sync_objects();
    }
    UIRenderer::~UIRenderer() {}

    void UIRenderer::create_pipeline()
    {
        // 作色器
        vk::raii::ShaderModule shader_module = create_shader_module(read_file("shaders/a.slang.spv"));

        vk::PipelineShaderStageCreateInfo vertex_shader_info{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shader_module,
            .pName = "vertMain"
        };

        vk::PipelineShaderStageCreateInfo fragment_shader_info{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shader_module,
            .pName = "fragMain"
        };

        vk::PipelineShaderStageCreateInfo shader_stages[] = {
            vertex_shader_info,
            fragment_shader_info
        };

        // 动态状态
        std::vector<vk::DynamicState> dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamic_state_info{
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates = dynamic_states.data()
        };

        // 顶点输入
        auto vertex_binding_description = Vertex::get_binding_description();
        auto vertex_attribute_description = Vertex::get_attribute_description();
        vk::PipelineVertexInputStateCreateInfo vertex_input_info{
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_binding_description,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_description.size()),
            .pVertexAttributeDescriptions = vertex_attribute_description.data()
        };
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{
            .topology = vk::PrimitiveTopology::eTriangleList
        };

        // 视口 与 裁剪
        vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swap_chain_.extent().width),
            .height = static_cast<float>(swap_chain_.extent().height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        vk::Rect2D scissor{
            .offset = vk::Offset2D{0, 0},
            .extent = swap_chain_.extent()
        };

        vk::PipelineViewportStateCreateInfo viewport_state{
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };

        // 光栅化
        vk::PipelineRasterizationStateCreateInfo rasterizer{
            .depthClampEnable        = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode             = vk::PolygonMode::eFill,
            .cullMode                = vk::CullModeFlagBits::eBack,
            .frontFace               = vk::FrontFace::eClockwise,
            .depthBiasEnable         = vk::False,
            .lineWidth               = 1.0f
        };

        // 多重采样
        vk::PipelineMultisampleStateCreateInfo multisampling{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False
        };

        // 颜色混合
        vk::PipelineColorBlendAttachmentState color_blend_attachment{
            .blendEnable    = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
        };

        vk::PipelineColorBlendStateCreateInfo color_blending{
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment
        };

        // 管线布局
        vk::PipelineLayoutCreateInfo pipeline_layout_info{
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0
        };

        pipeline_layout_ = vk::raii::PipelineLayout{context_.device(), pipeline_layout_info};

        // pipeline rendering
        // vk::PipelineRenderingCreateInfo rendering_info{
        //     .colorAttachmentCount = 1,
        //     .pColorAttachmentFormats = &swap_chain_.surface_format().format
        // };

        // 图形管线
        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_create_info_chain = {
            {.stageCount          = 2,
            .pStages             = shader_stages,
            .pVertexInputState   = &vertex_input_info,
            .pInputAssemblyState = &input_assembly_info,
            .pViewportState      = &viewport_state,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pColorBlendState    = &color_blending,
            .pDynamicState       = &dynamic_state_info,
            .layout              = pipeline_layout_,
            .renderPass          = nullptr},
            // pipeline rendering
            {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swap_chain_.surface_format().format}
        };

        pipeline_ = vk::raii::Pipeline{context_.device(), nullptr, pipeline_create_info_chain.get<vk::GraphicsPipelineCreateInfo>()};
    }

    void UIRenderer::create_command_pool()
    {
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = context_.queue_index()
        };

        command_pool_ = vk::raii::CommandPool{context_.device(), poolInfo};
    }

    void UIRenderer::create_vertex_buffer()
    {
        vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
        auto [staging_buffer, staging_buffer_memory] = create_buffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
        memcpy(data_staging, vertices.data(), (size_t)buffer_size);
        staging_buffer_memory.unmapMemory();

        std::tie(vertex_buffer_, vertex_buffer_memory_) = create_buffer(buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copy_buffer(staging_buffer, vertex_buffer_, buffer_size);
    }

    void UIRenderer::create_index_buffer()
    {
        vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();
        auto [staging_buffer, staging_buffer_memory] = create_buffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
        memcpy(data_staging, indices.data(), (size_t)buffer_size);
        staging_buffer_memory.unmapMemory();

        std::tie(index_buffer_, index_buffer_memory_) = create_buffer(buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copy_buffer(staging_buffer, index_buffer_, buffer_size);
    }

    void UIRenderer::copy_buffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size)
    {
        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = command_pool_,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffer command_copy_buffer = std::move(context_.device().allocateCommandBuffers(alloc_info).front());
        command_copy_buffer.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        command_copy_buffer.copyBuffer(*src, *dst, vk::BufferCopy{0, 0, size});
        command_copy_buffer.end();

        context_.graphics_queue().submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*command_copy_buffer}, nullptr);
        context_.graphics_queue().waitIdle();
    }

    uint32_t UIRenderer::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties)
    {
        vk::PhysicalDeviceMemoryProperties mem_properties = context_.physical_device().getMemoryProperties();
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> UIRenderer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
    {
        vk::BufferCreateInfo buffer_info{
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };
        vk::raii::Buffer buffer = vk::raii::Buffer(context_.device(), buffer_info);
        vk::MemoryRequirements mem_requirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo alloc_info{
            .allocationSize = mem_requirements.size,
            .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties)
        };
        vk::raii::DeviceMemory buffer_memory = vk::raii::DeviceMemory(context_.device(), alloc_info);
        buffer.bindMemory(*buffer_memory, 0);
        return {std::move(buffer), std::move(buffer_memory)};
    }

    void UIRenderer::create_command_buffers()
    {
        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = command_pool_,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };
        command_buffers_ = vk::raii::CommandBuffers(context_.device(), alloc_info);
    }

    void UIRenderer::create_sync_objects()
    {
        assert(present_complete_semaphores_.empty() && render_finished_semaphores_.empty() && in_flight_fences_.empty());
        for (size_t i = 0; i < swap_chain_.images().size(); i++)
            render_finished_semaphores_.emplace_back(context_.device(), vk::SemaphoreCreateInfo());
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            present_complete_semaphores_.emplace_back(context_.device(), vk::SemaphoreCreateInfo());
            in_flight_fences_.emplace_back(context_.device(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    void UIRenderer::draw_frame()
    {
        auto fence_result = context_.device().waitForFences(*in_flight_fences_[frame_index_], vk::True, UINT64_MAX);
        if (fence_result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to wait for fence!");
        }

        auto [result, image_index] = swap_chain_.handle().acquireNextImage(UINT64_MAX, *present_complete_semaphores_[frame_index_], nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            swap_chain_.recreate();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        // Only reset the fence if we are submitting work
        context_.device().resetFences(*in_flight_fences_[frame_index_]);

        command_buffers_[frame_index_].reset();
        record_command_buffer(image_index);

        vk::PipelineStageFlags wait_destination_stage_mask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
        const vk::SubmitInfo   submitInfo{
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &*present_complete_semaphores_[frame_index_],
            .pWaitDstStageMask    = &wait_destination_stage_mask,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &*command_buffers_[frame_index_],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &*render_finished_semaphores_[image_index]
        };

        context_.graphics_queue().submit(submitInfo, *in_flight_fences_[frame_index_]);

        const vk::PresentInfoKHR present_info_khr{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &*render_finished_semaphores_[image_index],
            .swapchainCount     = 1,
            .pSwapchains        = &*swap_chain_.handle(),
            .pImageIndices      = &image_index
        };
        result = context_.graphics_queue().presentKHR(present_info_khr);
        if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR || context_.window().was_window_resized())) {
            context_.window().reset_window_resized_flag();
            swap_chain_.recreate();
        } else {
            // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
            assert(result == vk::Result::eSuccess);
        }

        frame_index_ = (frame_index_ + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void UIRenderer::record_command_buffer(uint32_t image_index)
    {
        auto& command_buffer = command_buffers_[frame_index_];
        command_buffer.begin({});

		// Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
		transition_image_layout(
		    image_index,
		    vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    {},                                                        // srcAccessMask (no need to wait for previous operations)
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
		);
		vk::ClearValue              clear_color     = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::RenderingAttachmentInfo attachment_info = {
		    .imageView   = swap_chain_.image_views()[image_index],
		    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		    .loadOp      = vk::AttachmentLoadOp::eClear,
		    .storeOp     = vk::AttachmentStoreOp::eStore,
		    .clearValue  = clear_color};
		vk::RenderingInfo rendering_info = {
		    .renderArea           = {.offset = {0, 0}, .extent = swap_chain_.extent()},
		    .layerCount           = 1,
		    .colorAttachmentCount = 1,
		    .pColorAttachments    = &attachment_info};

		command_buffer.beginRendering(rendering_info);
		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);
        command_buffer.bindVertexBuffers(0, *vertex_buffer_, {0});
        command_buffer.bindIndexBuffer(*index_buffer_, 0, vk::IndexType::eUint16);
		command_buffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swap_chain_.extent().width), static_cast<float>(swap_chain_.extent().height), 0.0f, 1.0f));
		command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swap_chain_.extent()));
		// command_buffer.draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        command_buffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		command_buffer.endRendering();

		// After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
		transition_image_layout(
		    image_index,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    vk::ImageLayout::ePresentSrcKHR,
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
		    {},                                                        // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
		);
		command_buffer.end();
    }

    void UIRenderer::transition_image_layout(
            uint32_t                imageIndex,
            vk::ImageLayout         old_layout,
            vk::ImageLayout         new_layout,
            vk::AccessFlags2        src_access_mask,
            vk::AccessFlags2        dst_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dst_stage_mask)
    {
            vk::ImageMemoryBarrier2 barrier = {
                .srcStageMask        = src_stage_mask,
                .srcAccessMask       = src_access_mask,
                .dstStageMask        = dst_stage_mask,
                .dstAccessMask       = dst_access_mask,
                .oldLayout           = old_layout,
                .newLayout           = new_layout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = swap_chain_.images()[imageIndex],
                .subresourceRange    = {
                    .aspectMask     = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1}};
            vk::DependencyInfo dependency_info = {
                .dependencyFlags         = {},
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers    = &barrier};
        command_buffers_[frame_index_].pipelineBarrier2(dependency_info);
    }

    vk::raii::ShaderModule UIRenderer::create_shader_module(const std::vector<char>& code) const {
        vk::ShaderModuleCreateInfo create_info{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };

        vk::raii::ShaderModule shader_module{context_.device(), create_info};

        return shader_module;
    }

    std::vector<char> UIRenderer::read_file(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + filename);
        }
        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        file.close();
        return buffer;
    }
} // namespace xel::backend::vulkan