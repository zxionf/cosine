#pragma once

#include "../backend/device.hpp"
#include "../backend/swap_chain.hpp"

#include "draw_list.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>

namespace xel::ui
{
    class Renderer
    {
    public:
        Renderer(xel::backend::Device& device, xel::backend::SwapChain& swap_chain);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        void render(VkCommandBuffer command_buffer, DrawList& draw_list);

    private:

    };
} // namespace xel::ui