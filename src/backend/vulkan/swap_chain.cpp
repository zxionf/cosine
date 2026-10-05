#include "swap_chain.hpp"

#include <iostream>

namespace xel::backend::vulkan
{
    SwapChain::SwapChain(VulkanContext& context, Window& window) : context_{context}, window_{window}
    {
        create_swap_chain();
        create_image_views();
    }

    void SwapChain::cleanup()
    {
        swap_chain_image_views_.clear();
        swap_chain_ = nullptr;
    }

    void SwapChain::recreate()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window_.get_glfw_window(), &width, &height);
        while ((width == 0 || height == 0) && !window_.should_close()) {
            glfwGetFramebufferSize(window_.get_glfw_window(), &width, &height);
        }
        if (window_.should_close()) {
            return;
        }

        context_.device().waitIdle();

        cleanup();

        create_swap_chain();
        create_image_views();
    }

    void SwapChain::create_swap_chain()
    {
        vk::SurfaceCapabilitiesKHR surface_capabilities = context_.physical_device().getSurfaceCapabilitiesKHR(context_.surface());
        swap_chain_extent_ = choose_swap_chain_extent(surface_capabilities);
        uint32_t min_image_count = choose_swap_min_image_count(surface_capabilities);

        std::vector<vk::SurfaceFormatKHR> available_formats = context_.physical_device().getSurfaceFormatsKHR(context_.surface());
        swap_chain_surface_format_ = choose_swap_surface_format(available_formats);

        std::vector<vk::PresentModeKHR> available_present_modes = context_.physical_device().getSurfacePresentModesKHR(context_.surface());
        vk::PresentModeKHR present_mode = choose_swap_chain_present_mode(available_present_modes);

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{
            .surface       = context_.surface(),
            .minImageCount    = min_image_count,
            .imageFormat      = swap_chain_surface_format_.format,
            .imageColorSpace  = swap_chain_surface_format_.colorSpace,
            .imageExtent      = swap_chain_extent_,
            .imageArrayLayers = 1,
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform     = surface_capabilities.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = present_mode,
            .clipped          = true
        };

        swap_chain_ = vk::raii::SwapchainKHR{context_.device(), swapChainCreateInfo};
        swap_chain_images_ = swap_chain_.getImages();
    }

    void SwapChain::create_image_views()
    {
        assert(swap_chain_image_views_.empty());

        vk::ImageViewCreateInfo image_view_create_info{
            .viewType = vk::ImageViewType::e2D,
            .format   = swap_chain_surface_format_.format,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        };

        for (auto& image : swap_chain_images_) {
            image_view_create_info.image = image;
            swap_chain_image_views_.emplace_back(context_.device(), image_view_create_info);
        }
    }

    vk::SurfaceFormatKHR SwapChain::choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
        const auto format_it = std::ranges::find_if(available_formats, [](const auto& format){
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
        return format_it != available_formats.end() ? *format_it : available_formats[0];
    }

    vk::PresentModeKHR SwapChain::choose_swap_chain_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes) {
        // assert(std::ranges::any_of(available_present_modes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
        // return std::ranges::any_of(available_present_modes, [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
        //         vk::PresentModeKHR::eMailbox :
        //         vk::PresentModeKHR::eFifo;
        auto result = std::ranges::any_of(available_present_modes, [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
                vk::PresentModeKHR::eMailbox :
                vk::PresentModeKHR::eFifo;
        // if (result == vk::PresentModeKHR::eMailbox) std::cout << "present mode: Mailbox" << std::endl;
        // if (result == vk::PresentModeKHR::eFifo) std::cout << "present mode: FIFO" << std::endl;
        return result;
    }

    vk::Extent2D SwapChain::choose_swap_chain_extent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(window_.get_glfw_window(), &width, &height);
        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    uint32_t SwapChain::choose_swap_min_image_count(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        auto min_image_count = std::max(3u, capabilities.minImageCount);
        if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < min_image_count)) {
            min_image_count = capabilities.maxImageCount;
        }
        return min_image_count;
    }
}