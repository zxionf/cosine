#pragma once

#include "xel_device.hpp"
#include "xel_window.hpp"
#include "xel_game_object.hpp"
#include "xel_renderer.hpp"

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
        void loadGameModels();

        XelWindow window{WIDTH, HEIGHT, "hello vulkan"};
        XelDevice device{window};
        XelRenderer renderer{window, device};

        std::vector<XelGameObject> gameObjects;
        // XelPipeline pipeline{device,
        //     "shaders/simple_shader.vert.spv",
        //     "shaders/simple_shader.frag.spv",
        //     XelPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)
        // };
    };
}