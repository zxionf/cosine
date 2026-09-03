#pragma once

#include "xel_device.hpp"
#include "xel_pipeline.hpp"
#include "xel_swap_chain.hpp"
#include "xel_window.hpp"
#include "xel_model.hpp"

#include <memory>

namespace xel
{
    class FirstApp
    {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp &) = delete;
        FirstApp &operator=(const FirstApp&) = delete;

        void run();

    private:
        void loadModels();
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();

        XelWindow window{WIDTH, HEIGHT, "hello vulkan"};
        XelDevice device{window};
        XelSwapChain swapChain{device, window.getExtend()};
        std::unique_ptr<XelPipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<VkCommandBuffer> commandBuffers;
        std::unique_ptr<XelModel> model;
        // XelPipeline pipeline{device,
        //     "shaders/simple_shader.vert.spv",
        //     "shaders/simple_shader.frag.spv",
        //     XelPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)
        // };
    };
}