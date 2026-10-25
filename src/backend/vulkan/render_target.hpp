#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

namespace xel::backend::vulkan
{
// 渲染目标描述：只描述“画到哪里”，不拥有 image/view。
// 当前只构造 SwapChain 目标；离屏目标后续复用同一接口。
struct RenderTarget
{
    vk::Image       image{};
    vk::ImageView   view{};
    vk::Extent2D    extent{};
    vk::Format      format = vk::Format::eUndefined;
    vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
    vk::ImageLayout final_layout   = vk::ImageLayout::ePresentSrcKHR;
    bool            is_swapchain = false;
    uint32_t        image_index = 0;

    bool valid() const
    {
        return image != nullptr && view != nullptr && extent.width > 0 && extent.height > 0;
    }
};
} // namespace xel::backend::vulkan
