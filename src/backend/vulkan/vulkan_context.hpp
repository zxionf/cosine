#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include "window.hpp"

namespace xel::backend::vulkan
{
    class VulkanContext
    {
    public:
    private:
        void create_instance();

        vk::raii::Context context_;
        vk::raii::Instance instance_ = nullptr;
    };
} // namespace xel::backend::vk