#include "ui_renderer.hpp"

#include <fstream>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace xel::backend::vulkan
{
    UIRenderer::UIRenderer(VulkanContext& context, SwapChain& swap_chain) : context_{context}, swap_chain_{swap_chain}
    {
        create_descriptor_set_layout();
        create_pipeline();
        create_command_pool();
        create_texture_image();
        create_texture_image_view();
        create_texture_sampler();
        // vertices = ShapeMaker::makeRingVertices(0.5f, 0.25f, 16);
        // indices = ShapeMaker::makeRingIndices(vertices.size());
        create_vertex_buffer();
        create_index_buffer();
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
        create_command_buffers();
        create_sync_objects();
    }
    UIRenderer::~UIRenderer() {}

    void UIRenderer::create_descriptor_set_layout()
    {
        std::array<vk::DescriptorSetLayoutBinding, 2> ubo_layout_bindings{{
            {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex
            },{
                .binding = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment
            }
        }};
        vk::DescriptorSetLayoutCreateInfo layout_info{
            .bindingCount = static_cast<uint32_t>(ubo_layout_bindings.size()),
            .pBindings = ubo_layout_bindings.data()
        };
        descriptor_set_layout_ = vk::raii::DescriptorSetLayout{context_.device(), layout_info};
    }

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
            .cullMode                = vk::CullModeFlagBits::eNone,
            .frontFace               = vk::FrontFace::eCounterClockwise,
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
            .setLayoutCount = 1,
            .pSetLayouts = &*descriptor_set_layout_,
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

    void UIRenderer::create_texture_image()
    {
        int tex_width, tex_height, tex_channels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
        vk::DeviceSize image_size = tex_width * tex_height * 4;
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        auto [staging_buffer, staging_buffer_memory] = create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data = staging_buffer_memory.mapMemory(0, image_size);
        memcpy(data, pixels, (size_t)image_size);
        staging_buffer_memory.unmapMemory();
        stbi_image_free(pixels);

        std::tie(texture_image_, texture_image_memory_) = create_image(tex_width, tex_height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

        vk::raii::CommandBuffer command_buffer = begin_single_time_commands();
        transition_image_layout(command_buffer, texture_image_, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        copy_buffer_to_image(command_buffer, staging_buffer, texture_image_, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
        transition_image_layout(command_buffer, texture_image_, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        end_single_time_commands(std::move(command_buffer));
    }

    void UIRenderer::create_texture_sampler()
    {
        vk::PhysicalDeviceProperties properties = context_.physical_device().getProperties();
        vk::SamplerCreateInfo sampler_info{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
            .mipLodBias = 0.0f,
            .anisotropyEnable = properties.limits.maxSamplerAnisotropy > 1.0f ? vk::True : vk::False,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareEnable = vk::False,
            .compareOp = vk::CompareOp::eAlways,
            .minLod = 0.0f,
            .maxLod = 0.0f
        };
        texture_sampler_ = vk::raii::Sampler{context_.device(), sampler_info};
    }

    void UIRenderer::create_texture_image_view()
    {
        texture_image_view_ = create_image_view(*texture_image_, vk::Format::eR8G8B8A8Srgb);
    }

    vk::raii::ImageView UIRenderer::create_image_view(const vk::Image& image, vk::Format format)
    {
        // TODO
        /* 这个函数可以用来化简swapchain中create_image_views()函数*/
        vk::ImageViewCreateInfo view_info{
            .image   = image,
            .viewType = vk::ImageViewType::e2D,
            .format   = format,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
        };
        return vk::raii::ImageView{context_.device(), view_info};
    }

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> UIRenderer::create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
    {
        vk::ImageCreateInfo image_info{
            .imageType   = vk::ImageType::e2D,
            .format      = format,
            .extent      = vk::Extent3D{width, height, 1},
            .mipLevels   = 1,
            .arrayLayers = 1,
            .tiling      = tiling,
            .usage       = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };
        vk::raii::Image image = vk::raii::Image{context_.device(), image_info};

        vk::MemoryRequirements mem_requirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo alloc_info{
            .allocationSize = mem_requirements.size,
            .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties)
        };
        vk::raii::DeviceMemory image_memory = vk::raii::DeviceMemory{context_.device(), alloc_info};
        image.bindMemory(image_memory, 0);
        return {std::move(image), std::move(image_memory)};
    }

    void UIRenderer::copy_buffer_to_image(vk::raii::CommandBuffer& command_buffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height)
    {
        vk::BufferImageCopy region{
            .bufferOffset      = 0,
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource  = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
            .imageOffset       = vk::Offset3D{0, 0, 0},
            .imageExtent       = vk::Extent3D{width, height, 1}
        };
        command_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    }

    void UIRenderer::transition_image_layout(vk::raii::CommandBuffer& command_buffer, const vk::raii::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
    {
        vk::ImageMemoryBarrier barrier{
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = image,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}
        };
        vk::PipelineStageFlags source_stage;
        vk::PipelineStageFlags destination_stage;

        if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            source_stage      = vk::PipelineStageFlagBits::eTopOfPipe;
            destination_stage = vk::PipelineStageFlagBits::eTransfer;
        } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            source_stage      = vk::PipelineStageFlagBits::eTransfer;
            destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        command_buffer.pipelineBarrier(source_stage, destination_stage, {}, {}, nullptr, barrier);
    }

    vk::raii::CommandBuffer UIRenderer::begin_single_time_commands()
    {
        vk::CommandBufferAllocateInfo alloc_info{
            .commandPool = command_pool_,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffer command_buffer = std::move(vk::raii::CommandBuffers(context_.device(), alloc_info).front());

        vk::CommandBufferBeginInfo begin_info{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };
        command_buffer.begin(begin_info);

        return std::move(command_buffer);
    }

    void UIRenderer::end_single_time_commands(vk::raii::CommandBuffer&& command_buffer)
    {
        command_buffer.end();

        vk::SubmitInfo submit_info{
            .commandBufferCount = 1,
            .pCommandBuffers = &*command_buffer
        };
        context_.graphics_queue().submit(submit_info, nullptr);
        context_.graphics_queue().waitIdle();
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
        vk::raii::CommandBuffer command_copy_buffer = begin_single_time_commands();
        command_copy_buffer.copyBuffer(*src, *dst, vk::BufferCopy{.size = size});
        end_single_time_commands(std::move(command_copy_buffer));
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

    void UIRenderer::create_uniform_buffers()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DeviceSize buffer_size = sizeof(UniformBufferObject);
            auto [buffer, buffer_memory] = create_buffer(buffer_size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            uniform_buffers_.emplace_back(std::move(buffer));
            uniform_buffers_memory_.emplace_back(std::move(buffer_memory));
            uniform_buffers_mapped_.emplace_back(uniform_buffers_memory_.back().mapMemory(0, buffer_size));
        }
    }

    void UIRenderer::create_descriptor_pool()
    {
        std::array<vk::DescriptorPoolSize, 2> pool_size{{
            {
                .type = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
            },{
                .type = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
            }
        }};
        vk::DescriptorPoolCreateInfo pool_info{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .poolSizeCount = static_cast<uint32_t>(pool_size.size()),
            .pPoolSizes = pool_size.data()
        };
        descriptor_pool_ = vk::raii::DescriptorPool(context_.device(), pool_info);
    }

    void UIRenderer::create_descriptor_sets()
    {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptor_set_layout_);
        vk::DescriptorSetAllocateInfo alloc_info{
            .descriptorPool = descriptor_pool_,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data()
        };
        descriptor_sets_ = context_.device().allocateDescriptorSets(alloc_info);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo buffer_info{
                .buffer = uniform_buffers_[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };
            vk::DescriptorImageInfo image_info{
                .sampler = texture_sampler_,
                .imageView = texture_image_view_,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };
            std::array<vk::WriteDescriptorSet, 2> descriptor_write{{
                {
                    .dstSet          = descriptor_sets_[i],
                    .dstBinding      = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType  = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo     = &buffer_info
                },{
                    .dstSet          = descriptor_sets_[i],
                    .dstBinding      = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo      = &image_info
                }
            }};
            context_.device().updateDescriptorSets(descriptor_write, {});
        }
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

    void UIRenderer::update_uniform_buffer(uint32_t current_image)
    {
        static auto start_time = std::chrono::high_resolution_clock::now();

        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

        time /= 3.0f;

        UniformBufferObject ubo;
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swap_chain_.extent().width) / static_cast<float>(swap_chain_.extent().height), 0.1f, 10.0f);

        memcpy(uniform_buffers_mapped_[current_image], &ubo, sizeof(ubo));
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

        update_uniform_buffer(frame_index_);

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
		command_buffer.setViewport(0, vk::Viewport(0.0f, static_cast<float>(swap_chain_.extent().height), static_cast<float>(swap_chain_.extent().width), -static_cast<float>(swap_chain_.extent().height), 0.0f, 1.0f));
		command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swap_chain_.extent()));
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout_, 0, *descriptor_sets_[frame_index_], nullptr);
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