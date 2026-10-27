#include "renderer.hpp"
#include "../asset_path.hpp"
#include "../../graphics/triangle.hpp"
#include "../../graphics/font_atlas_builder.hpp"
#include "../../graphics/rectangle.hpp"

#include <fstream>
#include <chrono>
#include <iostream>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace xel::backend::vulkan
{
Renderer::Renderer(VulkanContext& context, SwapChain& swap_chain) : context_{context}, swap_chain_{swap_chain}
{
    create_descriptor_set_layout();
    create_pipeline();
    create_command_pool();
    create_white_texture();
    create_texture_image();
    create_texture_image_view();
    create_texture_sampler();
    create_dynamic_buffers();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();
}
Renderer::~Renderer()
{
    // 成员（swapchain / buffer / 同步对象）在本体之后析构；
    // 必须先等 GPU 完成，否则校验层会报 "in use by VkQueue / VkCommandBuffer"。
    context_.graphics_queue().waitIdle();
}

void Renderer::create_descriptor_set_layout()
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

void Renderer::create_pipeline()
{
    // 作色器
    vk::raii::ShaderModule shader_module = create_shader_module(read_file(require_asset("shaders/ui.slang.spv").string()));

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

    // 推送常量
    vk::PushConstantRange push_constant_range{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant)};

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
        .blendEnable    = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        // .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .alphaBlendOp        = vk::BlendOp::eAdd,
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
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range
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

void Renderer::create_command_pool()
{
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = context_.queue_index()
    };

    command_pool_ = vk::raii::CommandPool{context_.device(), poolInfo};
}

void Renderer::create_white_texture()
{
    uint8_t white_pixel[4] = {255, 255, 255, 255};
    vk::DeviceSize image_size = 4;

    auto [staging_buffer, staging_memory] = create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = staging_memory.mapMemory(0, image_size);
    memcpy(data, white_pixel, (size_t)image_size);
    staging_memory.unmapMemory();
    std::tie(white_image_, white_image_memory_) = create_image(1, 1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);
    // upload
    vk::raii::CommandBuffer cmd = begin_single_time_commands();
    transition_image_layout(cmd, white_image_, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copy_buffer_to_image(cmd, staging_buffer, white_image_, 1, 1);
    transition_image_layout(cmd, white_image_, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    end_single_time_commands(std::move(cmd));
    white_image_view_ = create_image_view(*white_image_, vk::Format::eR8G8B8A8Unorm);
}

void Renderer::create_texture_image()
{
    std::string font_error;
    const auto font = graphics::find_cjk_font();
    const auto coverage = find_asset("fonts/gb2312_level1.txt");

    if (!font.path.empty()) {
        graphics::FontAtlasBuildOptions options;
        options.font_path = font.path;
        options.face_index = font.face_index;
        options.pixel_size = 16;
        options.atlas_size = 2048;
        options.coverage_file = coverage.path.string();
        if (!coverage.found) {
            std::cerr << format_asset_error(coverage)
                      << "\nwarning: continuing CJK atlas with ASCII + punctuation only\n";
        }

        if (!graphics::build_font_atlas(options, font_atlas_, &font_error)) {
            std::cerr << "[Font] CJK atlas build failed: " << font_error
                      << "\n  font: " << font.path
                      << "\n  face: " << font.face_index
                      << "\n  coverage: " << coverage.path
                      << "\nwarning: falling back to default8.png\n";
        }
    }

    if (!font_atlas_.valid()) {
        const auto fallback = find_asset("fonts/default8.png");
        if (!fallback.found) {
            throw std::runtime_error(format_asset_error(fallback));
        }

        int tex_width = 0;
        int tex_height = 0;
        int tex_channels = 0;
        stbi_uc* pixels = stbi_load(fallback.path.string().c_str(),
                                    &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("[Font] failed to decode fallback atlas: " + fallback.path.string());
        }
        font_atlas_.load(tex_width, tex_height, pixels);
        stbi_image_free(pixels);
    }

    if (font_atlas_.smooth() && !font.path.empty()) {
        if (font_face_.load(font.path, font.face_index, 16)) {
            font_atlas_.set_glyph_loader([this](graphics::FontAtlas& atlas, char32_t codepoint, int pixel_size) {
                return font_face_.add_glyph(atlas, codepoint, pixel_size);
            });
        } else {
            std::cerr << "[Font] dynamic glyph loading disabled: "
                      << font.path << " (face " << font.face_index << ")\n";
        }
    }
    const uint32_t tex_width = static_cast<uint32_t>(font_atlas_.width());
    const uint32_t tex_height = static_cast<uint32_t>(font_atlas_.height());
    const vk::DeviceSize image_size = static_cast<vk::DeviceSize>(tex_width) * tex_height * 4;

    auto [staging_buffer, staging_buffer_memory] = create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = staging_buffer_memory.mapMemory(0, image_size);
    memcpy(data, font_atlas_.pixels().data(), static_cast<size_t>(image_size));
    staging_buffer_memory.unmapMemory();

    std::tie(texture_image_, texture_image_memory_) = create_image(tex_width, tex_height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::raii::CommandBuffer command_buffer = begin_single_time_commands();
    transition_image_layout(command_buffer, texture_image_, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copy_buffer_to_image(command_buffer, staging_buffer, texture_image_, tex_width, tex_height);
    transition_image_layout(command_buffer, texture_image_, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    end_single_time_commands(std::move(command_buffer));

    font_atlas_revision_ = font_atlas_.revision();
    graphics::set_active_font_atlas(&font_atlas_);
}

void Renderer::update_font_atlas_texture()
{
    if (font_atlas_revision_ == font_atlas_.revision()) return;
    font_atlas_revision_ = font_atlas_.revision();

    // 只上传自上次同步以来变化的矩形。新增字形只改局部区域，不再整张重传。
    int dx = 0;
    int dy = 0;
    int dw = 0;
    int dh = 0;
    if (!font_atlas_.take_dirty_rect(dx, dy, dw, dh)) return; // 只新增了无像素字形

    const auto width = static_cast<uint32_t>(font_atlas_.width());
    const auto row_bytes = static_cast<vk::DeviceSize>(dw) * 4;
    const vk::DeviceSize image_size = row_bytes * static_cast<vk::DeviceSize>(dh);

    auto [staging_buffer, staging_memory] = create_buffer(
        image_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    auto* data = static_cast<unsigned char*>(staging_memory.mapMemory(0, image_size));
    const auto& pixels = font_atlas_.pixels();
    for (int row = 0; row < dh; ++row) {
        const auto offset = (static_cast<std::size_t>(dy + row) * width + static_cast<std::size_t>(dx)) * 4;
        memcpy(data + static_cast<std::size_t>(row) * static_cast<std::size_t>(row_bytes),
               pixels.data() + offset,
               static_cast<std::size_t>(row_bytes));
    }
    staging_memory.unmapMemory();

    vk::raii::CommandBuffer command_buffer = begin_single_time_commands();
    transition_image_layout(command_buffer, texture_image_, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal);

    const vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {dx, dy, 0},
        .imageExtent = {static_cast<uint32_t>(dw), static_cast<uint32_t>(dh), 1},
    };
    command_buffer.copyBufferToImage(staging_buffer, texture_image_,
                                     vk::ImageLayout::eTransferDstOptimal, region);

    transition_image_layout(command_buffer, texture_image_, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    end_single_time_commands(std::move(command_buffer));
}

void Renderer::create_texture_sampler()
{
    vk::PhysicalDeviceProperties properties = context_.physical_device().getProperties();
    const vk::Filter filter = font_atlas_.smooth() ? vk::Filter::eLinear : vk::Filter::eNearest;
    vk::SamplerCreateInfo sampler_info{
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
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

void Renderer::create_texture_image_view()
{
    texture_image_view_ = create_image_view(*texture_image_, vk::Format::eR8G8B8A8Srgb);
}

vk::raii::ImageView Renderer::create_image_view(const vk::Image& image, vk::Format format)
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

std::pair<vk::raii::Image, vk::raii::DeviceMemory> Renderer::create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
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

void Renderer::copy_buffer_to_image(vk::raii::CommandBuffer& command_buffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height)
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

void Renderer::transition_image_layout(vk::raii::CommandBuffer& command_buffer, const vk::raii::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
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
    } else if (old_layout == vk::ImageLayout::eShaderReadOnlyOptimal && new_layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        source_stage      = vk::PipelineStageFlagBits::eFragmentShader;
        destination_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        source_stage      = vk::PipelineStageFlagBits::eTransfer;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("[Frame] unsupported image layout transition");
    }
    command_buffer.pipelineBarrier(source_stage, destination_stage, {}, {}, nullptr, barrier);
}

vk::raii::CommandBuffer Renderer::begin_single_time_commands()
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

    return command_buffer;
}

void Renderer::end_single_time_commands(vk::raii::CommandBuffer&& command_buffer)
{
    command_buffer.end();

    vk::SubmitInfo submit_info{
        .commandBufferCount = 1,
        .pCommandBuffers = &*command_buffer
    };
    context_.graphics_queue().submit(submit_info, nullptr);
    context_.graphics_queue().waitIdle();
}

// void Renderer::create_vertex_buffer()
// {
    // vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    // auto [staging_buffer, staging_buffer_memory] = create_buffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    // void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
    // memcpy(data_staging, vertices.data(), (size_t)buffer_size);
    // staging_buffer_memory.unmapMemory();

    // std::tie(vertex_buffer_, vertex_buffer_memory_) = create_buffer(buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
    // copy_buffer(staging_buffer, vertex_buffer_, buffer_size);
// }

// void Renderer::create_index_buffer()
// {
    // vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    // auto [staging_buffer, staging_buffer_memory] = create_buffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    // void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
    // memcpy(data_staging, indices.data(), (size_t)buffer_size);
    // staging_buffer_memory.unmapMemory();

    // std::tie(index_buffer_, index_buffer_memory_) = create_buffer(buffer_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
    // copy_buffer(staging_buffer, index_buffer_, buffer_size);
// }

void Renderer::copy_buffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size)
{
    vk::raii::CommandBuffer command_copy_buffer = begin_single_time_commands();
    command_copy_buffer.copyBuffer(*src, *dst, vk::BufferCopy{.size = size});
    end_single_time_commands(std::move(command_copy_buffer));
}

uint32_t Renderer::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties mem_properties = context_.physical_device().getMemoryProperties();
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("[Vulkan] failed to find a suitable memory type");
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Renderer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
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

void Renderer::create_uniform_buffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize buffer_size = sizeof(UniformBufferObject);
        auto [buffer, buffer_memory] = create_buffer(buffer_size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        uniform_buffers_.emplace_back(std::move(buffer));
        uniform_buffers_memory_.emplace_back(std::move(buffer_memory));
        uniform_buffers_mapped_.emplace_back(uniform_buffers_memory_.back().mapMemory(0, buffer_size));
    }
}

void Renderer::create_descriptor_pool()
{
    std::array<vk::DescriptorPoolSize, 2> pool_size{{
        {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2
        },{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2
        }
    }};
    vk::DescriptorPoolCreateInfo pool_info{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2,
        .poolSizeCount = static_cast<uint32_t>(pool_size.size()),
        .pPoolSizes = pool_size.data()
    };
    descriptor_pool_ = vk::raii::DescriptorPool(context_.device(), pool_info);
}

void Renderer::create_descriptor_sets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT * 2, *descriptor_set_layout_);
    vk::DescriptorSetAllocateInfo alloc_info{
        .descriptorPool = descriptor_pool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };
    // descriptor_sets_ = context_.device().allocateDescriptorSets(alloc_info);
    descriptor_sets_ = context_.device().allocateDescriptorSets(alloc_info);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo buffer_info{
            .buffer = uniform_buffers_[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };
        vk::DescriptorImageInfo white_info{
            .sampler = texture_sampler_,
            .imageView = white_image_view_,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };
        vk::DescriptorImageInfo image_info{
            .sampler = texture_sampler_,
            .imageView = texture_image_view_,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        auto& white_set = descriptor_sets_[i * 2 + static_cast<uint32_t>(graphics::TextureSlot::White)];
        auto& texture_set = descriptor_sets_[i * 2 + static_cast<uint32_t>(graphics::TextureSlot::Texture)];
        std::array<vk::WriteDescriptorSet, 4> descriptor_write{{
            {
                .dstSet          = texture_set,
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo     = &buffer_info
            },{
                .dstSet          = texture_set,
                .dstBinding      = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo      = &image_info
            },{
                .dstSet          = white_set,
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo     = &buffer_info
            },{
                .dstSet          = white_set,
                .dstBinding      = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo      = &white_info
            }
        }};
        context_.device().updateDescriptorSets(descriptor_write, {});
    }
}

void Renderer::create_command_buffers()
{
    vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = command_pool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    command_buffers_ = vk::raii::CommandBuffers(context_.device(), alloc_info);
}

void Renderer::create_sync_objects()
{
    assert(present_complete_semaphores_.empty() && render_finished_semaphores_.empty() && in_flight_fences_.empty());
    recreate_render_finished_semaphores();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        present_complete_semaphores_.emplace_back(context_.device(), vk::SemaphoreCreateInfo());
        in_flight_fences_.emplace_back(context_.device(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void Renderer::recreate_render_finished_semaphores()
{
    // 每个交换链图像对应一个信号量，交换链重建后图像数量可能变化，必须同步重建
    render_finished_semaphores_.clear();
    render_finished_semaphores_.reserve(swap_chain_.images().size());
    for (size_t i = 0; i < swap_chain_.images().size(); i++) {
        render_finished_semaphores_.emplace_back(context_.device(), vk::SemaphoreCreateInfo());
    }
}

void Renderer::update_push_constants()
{
    const vk::Extent2D extent = active_target_.valid() ? active_target_.extent : swap_chain_.extent();
    // UI 逻辑坐标 → NDC；viewport 仍是物理像素，因此整体被放大 ui_scale_ 倍。
    const float logical_w = static_cast<float>(extent.width) / ui_scale_;
    const float logical_h = static_cast<float>(extent.height) / ui_scale_;
    PushConstant push_constant{
        .scale      = glm::vec2(2.0f / logical_w, 2.0f / logical_h),
        .translate  = glm::vec2(-1.0f,                -1.0f),
        .time = static_cast<float>(glfwGetTime())
    };
    command_buffers_[frame_index_].pushConstants<PushConstant>(pipeline_layout_, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, push_constant);
}

void Renderer::update_uniform_buffer()
{
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    time /= 3.0f;

    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    const vk::Extent2D extent = active_target_.valid() ? active_target_.extent : swap_chain_.extent();
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 10.0f);
    memcpy(uniform_buffers_mapped_[frame_index_], &ubo, sizeof(ubo));
}

void Renderer::draw(const graphics::Drawable& d)
{
    draw_list_.set_texture(d.texture_slot());
    d.append_to(draw_list_);
}

void Renderer::set_scissor(float x, float y, float w, float h)
{
    draw_list_.set_clip({x, y, w, h});
}

void Renderer::reset_scissor()
{
    const vk::Extent2D extent = active_target_.valid() ? active_target_.extent : swap_chain_.extent();
    draw_list_.set_full_clip({0.0f, 0.0f,
                              static_cast<float>(extent.width),
                              static_cast<float>(extent.height)});
}
void Renderer::draw_line(float x1, float y1, float x2, float y2, float width, glm::vec4 color)
{
    draw_list_.set_texture(graphics::TextureSlot::White);
    // 方向
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f) return;  // 退化线段，跳过

    // 单位方向 + 垂直方向（乘以半宽）
    float ux = dx / len;
    float uy = dy / len;
    float half_w = width * 0.5f;
    float px = -uy * half_w;
    float py =  ux * half_w;

    // 四个角点
    glm::vec2 a{x1 + px, y1 + py};
    glm::vec2 b{x1 - px, y1 - py};
    glm::vec2 c{x2 - px, y2 - py};
    glm::vec2 d{x2 + px, y2 + py};

    uint32_t base = static_cast<uint32_t>(draw_list_.batch().vertices.size());

    // 4 个顶点，UV 随便填（用白纹理采样恒为 1）
    draw_list_.batch().vertices.push_back({{a.x, a.y}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{b.x, b.y}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{c.x, c.y}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{d.x, d.y}, color, {0.0f, 0.0f}});

    // 两个三角形
    draw_list_.batch().indices.insert(draw_list_.batch().indices.end(), {
        base, static_cast<uint32_t>(base + 1), static_cast<uint32_t>(base + 2),
        base, static_cast<uint32_t>(base + 2), static_cast<uint32_t>(base + 3)
    });
}

void Renderer::draw_rect(float x, float y, float w, float h, glm::vec4 color)
{
    draw_list_.set_texture(graphics::TextureSlot::White);
    uint32_t base = static_cast<uint32_t>(draw_list_.batch().vertices.size());
    // 0  3
    // 1  2
    draw_list_.batch().vertices.push_back({{x,         y}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{x + w,     y}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{x + w, y + h}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{x,     y + h}, color, {0.0f, 0.0f}});
    draw_list_.batch().indices.insert(draw_list_.batch().indices.end(), {
        base, static_cast<uint32_t>(base + 1), static_cast<uint32_t>(base + 2),
        static_cast<uint32_t>(base + 2), static_cast<uint32_t>(base + 3), base
    });
}

void Renderer::draw_rect_uv(float x, float y, float w, float h, glm::vec4 color, float u0, float v0, float u1, float v1)
{
    draw_list_.set_texture(graphics::TextureSlot::Texture);
    uint32_t base = static_cast<uint32_t>(draw_list_.batch().vertices.size());
    // 0  3
    // 1  2
    draw_list_.batch().vertices.push_back({{x,         y}, color, {u0, v0}});
    draw_list_.batch().vertices.push_back({{x + w,     y}, color, {u1, v0}});
    draw_list_.batch().vertices.push_back({{x + w, y + h}, color, {u1, v1}});
    draw_list_.batch().vertices.push_back({{x,     y + h}, color, {u0, v1}});
    draw_list_.batch().indices.insert(draw_list_.batch().indices.end(), {
        base, static_cast<uint32_t>(base + 1), static_cast<uint32_t>(base + 2),
        static_cast<uint32_t>(base + 2), static_cast<uint32_t>(base + 3), base
    });
}

void Renderer::draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, glm::vec4 color)
{
    draw_list_.set_texture(graphics::TextureSlot::White);
    uint32_t base = static_cast<uint32_t>(draw_list_.batch().vertices.size());
    draw_list_.batch().vertices.push_back({{x1, y1}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{x2, y2}, color, {0.0f, 0.0f}});
    draw_list_.batch().vertices.push_back({{x3, y3}, color, {0.0f, 0.0f}});
    draw_list_.batch().indices.insert(draw_list_.batch().indices.end(), {
        base, static_cast<uint32_t>(base + 1), static_cast<uint32_t>(base + 2)
    });
}

void Renderer::draw_char(float x, float y, float size, char ch, glm::vec4 color)
{
    draw_list_.set_texture(graphics::TextureSlot::Texture);
    const char text[2] = {ch, '\0'};
    font_atlas_.append_text(draw_list_, x, y, size, std::string_view{text, 1}, color);
}

void Renderer::draw_text(float x, float y, float size, const std::string& text, glm::vec4 color)
{
    draw_list_.set_texture(graphics::TextureSlot::Texture);
    font_atlas_.append_text(draw_list_, x, y, size, text, color);
}

RenderTarget Renderer::make_swapchain_target()
{
    RenderTarget target;
    target.image = swap_chain_.images()[image_index_];
    target.view = *swap_chain_.image_views()[image_index_];
    target.extent = swap_chain_.extent();
    target.format = swap_chain_.surface_format().format;
    target.initial_layout = vk::ImageLayout::eUndefined;
    target.final_layout = vk::ImageLayout::ePresentSrcKHR;
    target.is_swapchain = true;
    target.image_index = image_index_;
    return target;
}

void Renderer::begin()
{
    const auto now = std::chrono::steady_clock::now();
    if (has_last_frame_) {
        const double frame_ms = std::chrono::duration<double, std::milli>(now - last_frame_time_).count();
        frame_stats_.frame_time_ms = frame_ms;
        frame_stats_.fps = frame_ms > 0.0 ? 1000.0 / frame_ms : 0.0;
    }
    last_frame_time_ = now;
    has_last_frame_ = true;

    begin_frame();

    // 当前只有 SwapChain 目标；离屏目标后续通过同一 RenderTarget 接口接入。
    active_target_ = frame_acquired_ ? make_swapchain_target() : RenderTarget{};
    draw_list_.clear();
    reset_scissor();
}

void Renderer::end()
{
    // 本帧没有成功获取交换链图像（例如交换链已过期），不能提交任何工作
    if (!frame_acquired_) {
        return;
    }

    const RenderTarget& target = active_target_;

    frame_stats_.draw = graphics::collect_draw_stats(
        draw_list_,
        static_cast<float>(target.extent.width),
        static_cast<float>(target.extent.height));

    update_font_atlas_texture();
    update_uniform_buffer();

    command_buffers_[frame_index_].reset();
    // record_command_buffer(current_image_index_);
    auto& command_buffer = command_buffers_[frame_index_];
    command_buffer.begin({});
    update_push_constants();
    upload_vertex_data();
    begin_rendering(command_buffer, target);
    // command_buffer.bindVertexBuffers(0, *vertex_buffer_, {0});
    // command_buffer.bindIndexBuffer(*index_buffer_, 0, vk::IndexType::eUint16);
    // command_buffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    // if (current_vertex_buffer_size_ > 0) {
    //     command_buffer.bindVertexBuffers(0, *device_vertex_buffers_[frame_index_], {0});
    //     command_buffer.bindIndexBuffer(*device_index_buffers_[frame_index_], 0, vk::IndexTypeValue<decltype(draw_list_.batch().indices)::value_type>::value);
    //     command_buffer.drawIndexed(static_cast<uint32_t>(draw_list_.batch().indices.size()), 1, 0, 0, 0);
    // }
    for (const auto& batch : draw_list_.batches()) {
        if (batch.indices.empty()) continue;
        // UI 层已把裁剪矩形对齐到像素网格，这里取最近整数即可：
        // 向外取整（floor/ceil）会让 scissor 比控件大最多 1px，
        // 表现在屏幕上就是控件顶部/底部多出一条像素。
        const float sx0 = batch.scissor.x;
        const float sy0 = batch.scissor.y;
        const float sx1 = sx0 + std::max(0.0f, batch.scissor.z);
        const float sy1 = sy0 + std::max(0.0f, batch.scissor.w);
        const int x0 = std::max(0, static_cast<int>(std::lround(sx0 * ui_scale_)));
        const int y0 = std::max(0, static_cast<int>(std::lround(sy0 * ui_scale_)));
        const int x1 = std::min(static_cast<int>(target.extent.width), static_cast<int>(std::lround(sx1 * ui_scale_)));
        const int y1 = std::min(static_cast<int>(target.extent.height), static_cast<int>(std::lround(sy1 * ui_scale_)));
        const uint32_t scissor_w = static_cast<uint32_t>(std::max(0, x1 - x0));
        const uint32_t scissor_h = static_cast<uint32_t>(std::max(0, y1 - y0));
        // 空裁剪区不提交绘制：某些驱动对 extent=0 的 scissor 行为不一致，
        // 可能沿用上一个 scissor 而导致已滚出视区的内容露出。
        if (scissor_w == 0 || scissor_h == 0) continue;

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout_, 0,
            *descriptor_sets_[frame_index_ * 2 + static_cast<uint32_t>(batch.texture_slot)], nullptr);
        vk::Rect2D scissor{
            .offset = {x0, y0},
            .extent = {scissor_w, scissor_h}
        };
        command_buffer.setScissor(0, scissor);
        command_buffer.bindVertexBuffers(0, *device_vertex_buffers_[frame_index_], {batch.vertex_offset});
        command_buffer.bindIndexBuffer(*device_index_buffers_[frame_index_], batch.index_offset, vk::IndexType::eUint32);
        command_buffer.drawIndexed(static_cast<uint32_t>(batch.indices.size()), 1, 0, 0, 0);
    }

    end_rendering(command_buffer, target);
    command_buffer.end();
    end_frame();
}

void Renderer::begin_frame()
{
    frame_acquired_ = false;

    auto fence_result = context_.device().waitForFences(*in_flight_fences_[frame_index_], vk::True, UINT64_MAX);
    if (fence_result != vk::Result::eSuccess) {
        throw std::runtime_error("[Frame] failed to wait for in-flight fence");
    }

    auto [result, index] = swap_chain_.handle().acquireNextImage(UINT64_MAX, *present_complete_semaphores_[frame_index_], nullptr);

    // 注意：只有 eSuccess / eSuboptimalKHR 时 index 才是有效的；
    // 其他结果（尤其是 eErrorOutOfDateKHR）下驱动不会写入 index，其值是未初始化的垃圾值，
    // 必须在检查 result 之前避免使用它。
    if (result == vk::Result::eErrorOutOfDateKHR) {
        swap_chain_.recreate();
        if (context_.window().should_close()) {
            return;
        }
        // recreate() 内部已 waitIdle，此时重建每图像信号量是安全的
        recreate_render_finished_semaphores();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("[Frame] failed to acquire swap chain image");
    }

    image_index_ = index;

    // 防御性检查，确保后续按 image_index_ 访问的数组不会越界
    if (image_index_ >= swap_chain_.images().size() ||
        image_index_ >= swap_chain_.image_views().size() ||
        image_index_ >= render_finished_semaphores_.size()) {
        throw std::runtime_error("[Frame] acquired swap chain image index is out of range");
    }

    frame_acquired_ = true;

    // Only reset the fence if we are submitting work
    context_.device().resetFences(*in_flight_fences_[frame_index_]);
}

void Renderer::end_frame()
{
    vk::PipelineStageFlags wait_destination_stage_mask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    const vk::SubmitInfo   submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*present_complete_semaphores_[frame_index_],
        .pWaitDstStageMask    = &wait_destination_stage_mask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*command_buffers_[frame_index_],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*render_finished_semaphores_[image_index_]
    };

    context_.graphics_queue().submit(submitInfo, *in_flight_fences_[frame_index_]);


    const vk::PresentInfoKHR present_info_khr{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*render_finished_semaphores_[image_index_],
        .swapchainCount     = 1,
        .pSwapchains        = &*swap_chain_.handle(),
        .pImageIndices      = &image_index_
    };
    auto result = context_.graphics_queue().presentKHR(present_info_khr);
    if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR || context_.window().was_window_resized())) {
        context_.window().reset_window_resized_flag();
        swap_chain_.recreate();
        if (!context_.window().should_close()) {
            recreate_render_finished_semaphores();
        }
    } else {
        // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
        assert(result == vk::Result::eSuccess);
    }

    frame_index_ = (frame_index_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::end_rendering(vk::raii::CommandBuffer& command_buffer, const RenderTarget& target)
{
    command_buffer.endRendering();

    // SwapChain 目标 present；离屏目标转成可采样或可拷贝布局。
    vk::AccessFlags2        dst_access{};
    vk::PipelineStageFlags2 dst_stage = vk::PipelineStageFlagBits2::eBottomOfPipe;
    if (target.final_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        dst_access = vk::AccessFlagBits2::eShaderRead;
        dst_stage  = vk::PipelineStageFlagBits2::eFragmentShader;
    } else if (target.final_layout == vk::ImageLayout::eTransferSrcOptimal) {
        dst_access = vk::AccessFlagBits2::eTransferRead;
        dst_stage  = vk::PipelineStageFlagBits2::eTransfer;
    }

    transition_image_layout(
        target,
        vk::ImageLayout::eColorAttachmentOptimal,
        target.final_layout,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        dst_access,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        dst_stage
    );
}

void Renderer::begin_rendering(vk::raii::CommandBuffer& command_buffer, const RenderTarget& target)
{
    transition_image_layout(
        target,
        target.initial_layout,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                        // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
    );
    vk::ClearValue clear_color = vk::ClearColorValue(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);
    vk::RenderingAttachmentInfo attachment_info = {
        .imageView   = target.view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clear_color};
    vk::RenderingInfo rendering_info = {
        .renderArea           = {.offset = {0, 0}, .extent = target.extent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachment_info};

    command_buffer.beginRendering(rendering_info);
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);

    float windowW = static_cast<float>(target.extent.width);
    float windowH = static_cast<float>(target.extent.height);
    command_buffer.setViewport(0, vk::Viewport(0, 0, windowW, windowH, 0.0f, 1.0f));
    command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), target.extent));
}

void Renderer::create_dynamic_buffers()
{
    vk::DeviceSize vtx_size = sizeof(Vertex) * MAX_VERTICES_;
    vk::DeviceSize idx_size = sizeof(uint32_t) * MAX_INDICES_;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        {
            // vertex staging
            auto [buf, mem] = create_buffer(vtx_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            staging_vertex_buffers_.push_back(std::move(buf));
            staging_vertex_memories_.push_back(std::move(mem));
            staging_vertex_mapped_.push_back(staging_vertex_memories_.back().mapMemory(0, vtx_size));
        }{
            // vertex device-local
            auto [buf, mem] = create_buffer(vtx_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
            device_vertex_buffers_.push_back(std::move(buf));
            device_vertex_memories_.push_back(std::move(mem));
        }{
            // index staging
            auto [buf, mem] = create_buffer(idx_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            staging_index_buffers_.push_back(std::move(buf));
            staging_index_memories_.push_back(std::move(mem));
            staging_index_mapped_.push_back(staging_index_memories_.back().mapMemory(0, idx_size));
        }{
            auto [buf, mem] = create_buffer(idx_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
            device_index_buffers_.push_back(std::move(buf));
            device_index_memories_.push_back(std::move(mem));
        }
    }
}

void Renderer::upload_vertex_data()
{
    if (draw_list_.batches().empty()) return;
    vk::DeviceSize total_vertex_bytes = 0;
    vk::DeviceSize total_index_bytes  = 0;
    for (auto& batch : draw_list_.batches()) {
        batch.vertex_offset = static_cast<uint32_t>(total_vertex_bytes);
        batch.index_offset  = static_cast<uint32_t>(total_index_bytes);
        total_vertex_bytes += batch.vertices.size() * sizeof(Vertex);
        total_index_bytes  += batch.indices.size()  * sizeof(uint32_t);
    }
    if (total_vertex_bytes == 0 || total_index_bytes == 0) return;
    uint8_t* vtx_dst = static_cast<uint8_t*>(staging_vertex_mapped_[frame_index_]);
    uint8_t* idx_dst = static_cast<uint8_t*>(staging_index_mapped_[frame_index_]);

    for (const auto& batch : draw_list_.batches()) {
        size_t vtx_size = batch.vertices.size() * sizeof(Vertex);
        size_t idx_size = batch.indices.size()  * sizeof(uint32_t);
        memcpy(vtx_dst + batch.vertex_offset, batch.vertices.data(), vtx_size);
        memcpy(idx_dst + batch.index_offset,  batch.indices.data(),  idx_size);
    }

    command_buffers_[frame_index_].copyBuffer(*staging_vertex_buffers_[frame_index_], *device_vertex_buffers_[frame_index_], vk::BufferCopy{.size = total_vertex_bytes});
    command_buffers_[frame_index_].copyBuffer(*staging_index_buffers_[frame_index_], *device_index_buffers_[frame_index_], vk::BufferCopy{.size = total_index_bytes});

    vk::BufferMemoryBarrier2 barriers[2] = {
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask  = vk::PipelineStageFlagBits2::eVertexInput,
            .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = *device_vertex_buffers_[frame_index_],
            .offset = 0,
            .size   = total_vertex_bytes
        },
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask  = vk::PipelineStageFlagBits2::eVertexInput,
            .dstAccessMask = vk::AccessFlagBits2::eIndexRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = *device_index_buffers_[frame_index_],
            .offset = 0,
            .size   = total_index_bytes
        }
    };
    vk::DependencyInfo dep_info = {.bufferMemoryBarrierCount = 2, .pBufferMemoryBarriers = barriers};
    command_buffers_[frame_index_].pipelineBarrier2(dep_info);
}

void Renderer::transition_image_layout(
        const RenderTarget&     target,
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
            .image               = target.image,
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

vk::raii::ShaderModule Renderer::create_shader_module(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo create_info{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    vk::raii::ShaderModule shader_module{context_.device(), create_info};

    return shader_module;
}

std::vector<char> Renderer::read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("[File] failed to open: " + filename + "\nhint: ensure the shader was built into assets/shaders.");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
}
} // namespace xel::backend::vulkan
