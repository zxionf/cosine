#pragma once

#include "xel_window.hpp"
#include "xel_pipeline.hpp"
#include "xel_device.hpp"

namespace xel
{
    class FirstApp
    {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;
        void run();

    private:
        XelWindow window{WIDTH, HEIGHT, "hello vulkan"};
        XelDevice device{window};
        XelPipeline pipeline{device,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            XelPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)
        };
    };
}