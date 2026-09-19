#include "backend/device.hpp"
#include "backend/swap_chain.hpp"
#include "backend/xel_pipeline.hpp"
#include "xel_model.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <array>
#include <vector>

int main()
{
    struct SimplePushConstantData
    {
        glm::mat2 transform{1.f};
        glm::vec2 offset;
        alignas(16) glm::vec3 color;
    };
    using namespace xel::backend;
    Window window{800, 600, "Xel"};
    Device device{window};

    std::unique_ptr<SwapChain> swap_chain = std::make_unique<SwapChain>(device, window.get_extent());;
    std::vector<VkCommandBuffer> command_buffers;
    uint32_t current_image_index = 0;

    // create command buffers
    command_buffers.resize(swap_chain->image_count());
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = device.get_command_pool();
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
    if (vkAllocateCommandBuffers(device.device(), &alloc_info, command_buffers.data()) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate command buffers!");

    auto recreate_swap_chain = [&]() {
        vkDeviceWaitIdle(device.device());

        std::unique_ptr<SwapChain> old_swap_chain = std::move(swap_chain);
        // 如果你的 SwapChain 构造函数要 shared_ptr，就构造一个传进去
        swap_chain = std::make_unique<SwapChain>(device, window.get_extent(),
                                                std::shared_ptr<SwapChain>(std::move(old_swap_chain)));

        // 释放旧的命令缓冲区（GPU 已经 idle）
        vkFreeCommandBuffers(device.device(), device.get_command_pool(),
                            static_cast<uint32_t>(command_buffers.size()),
                            command_buffers.data());

        // 按新 image_count 重新分配
        command_buffers.resize(swap_chain->image_count());
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = device.get_command_pool();
        alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
        if (vkAllocateCommandBuffers(device.device(), &alloc_info, command_buffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers!");
    };

    // create pipeline layout
    VkPipelineLayout pipeline_layout;
    VkPushConstantRange push_constant_range{};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push_constant_range.size = sizeof(SimplePushConstantData);
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    if (vkCreatePipelineLayout(device.device(), &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout!");

    // create pipeline
    PipelineConfigInfo pipeline_config_info{};
    XelPipeline::defaultPipelineConfigInfo(pipeline_config_info);
    pipeline_config_info.renderPass = swap_chain->get_render_pass();
    pipeline_config_info.pipelineLayout = pipeline_layout;
    std::unique_ptr<XelPipeline> pipeline = std::make_unique<XelPipeline>(device, "shaders/simple_shader.vert.spv",
                                                                          "shaders/simple_shader.frag.spv",
                                                                          pipeline_config_info);

    // shape
    std::vector<xel::XelModel::Vertex> vertices
        {
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
    auto model = std::make_shared<xel::XelModel>(device, vertices);
    
    while (!window.should_close())
    {
        glfwPollEvents();

        if (window.was_window_resized())
        {
            recreate_swap_chain();
            window.reset_window_resized_flag();
        }

        // begin frame
        auto result = swap_chain->acquire_next_image(&current_image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreate_swap_chain();
            continue;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("failed to acquire swap chain image!");
        VkCommandBuffer command_buffer = command_buffers[current_image_index];
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer!");

        // begin render pass
        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = swap_chain->get_render_pass();
        render_pass_info.framebuffer = swap_chain->get_framebuffer(current_image_index);
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swap_chain->get_swap_chain_extent();
        std::array<VkClearValue, 2> clear_values{};
        clear_values[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
        clear_values[1].depthStencil = {1.0f, 0};
        render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_info.pClearValues = clear_values.data();
        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swap_chain->get_swap_chain_extent().width);
        viewport.height = static_cast<float>(swap_chain->get_swap_chain_extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, swap_chain->get_swap_chain_extent()};
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        // render
        pipeline->bind(command_buffer);

        SimplePushConstantData push{};
        push.offset = glm::vec2{0.0f, 0.0f};
        push.color = glm::vec3{1.0f, 1.0f, 1.0f};
        push.transform = glm::mat2(1.0f);

        vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
        model->bind(command_buffer);
        model->draw(command_buffer);

        // end render pass
        vkCmdEndRenderPass(command_buffer);
        // end frame
        auto command_buffer_result = command_buffers[current_image_index];
        if (vkEndCommandBuffer(command_buffer_result) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");
        auto result2 = swap_chain->submit_command_buffers(&command_buffer_result, &current_image_index);
        if (result2 == VK_ERROR_OUT_OF_DATE_KHR || result2 == VK_SUBOPTIMAL_KHR || window.was_window_resized())
        {
            window.reset_window_resized_flag();
            std::shared_ptr<SwapChain> old_swap_chain = std::move(swap_chain);
            swap_chain = std::make_unique<SwapChain>(device, window.get_extent(), old_swap_chain);
            continue;
        }
        if (result2 != VK_SUCCESS)
            throw std::runtime_error("failed to present swap chain image!");
    }

    vkDeviceWaitIdle(device.device());
    return 0;
}