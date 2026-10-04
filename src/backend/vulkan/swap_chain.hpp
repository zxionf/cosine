#pragma once

#include "vulkan_context.hpp"

namespace xel::backend::vulkan
{
    class SwapChain
    {
    public:
        SwapChain(VulkanContext& context, Window& window);

        vk::Extent2D extent() const { return swap_chain_extent_; }
        auto& surface_format()      { return swap_chain_surface_format_; }
        auto& images()              { return swap_chain_images_; }
        auto& image_views()         { return swap_chain_image_views_; }
        auto& handle()              { return swap_chain_; }

        void recreate();
        void cleanup();

    private:
        void create_swap_chain();
        void create_image_views();

        vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats);
        vk::PresentModeKHR choose_swap_chain_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes);
        vk::Extent2D choose_swap_chain_extent(const vk::SurfaceCapabilitiesKHR& capabilities);
        uint32_t choose_swap_min_image_count(const vk::SurfaceCapabilitiesKHR& capabilities);


        vk::raii::SwapchainKHR swap_chain_ = nullptr;
        std::vector<vk::Image> swap_chain_images_;
        vk::SurfaceFormatKHR   swap_chain_surface_format_;
        vk::Extent2D           swap_chain_extent_;

        std::vector<vk::raii::ImageView> swap_chain_image_views_;

        VulkanContext& context_;
        Window& window_;
    };
} // namespace xel::backend::vulkan