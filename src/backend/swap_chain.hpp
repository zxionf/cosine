#pragma once

#include "device.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>
#include <string>

namespace xel::backend
{
    class SwapChain
    {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SwapChain(Device &device, VkExtent2D window_extent);
        SwapChain(Device &device, VkExtent2D window_extent, std::shared_ptr<SwapChain> previous);
        ~SwapChain();

        SwapChain(const SwapChain &) = delete;
        SwapChain& operator=(const SwapChain&) = delete;

        VkFramebuffer get_framebuffer(int index) { return swap_chain_framebuffers_[index]; }
        VkRenderPass get_render_pass() { return render_pass_; }
        VkImageView get_image_view(int index) { return swap_chain_image_views_[index]; }
        size_t image_count() { return swap_chain_images_.size(); }
        VkFormat get_swap_chain_image_format() { return swap_chain_image_format_; }
        VkExtent2D get_swap_chain_extent() { return swap_chain_extent_; }
        uint32_t width() { return swap_chain_extent_.width; }
        uint32_t height() { return swap_chain_extent_.height; }
        VkSwapchainKHR get_swap_chain_handle() { return swap_chain_; }

        float extent_aspect_ratio() { return static_cast<float>(swap_chain_extent_.width) / static_cast<float>(swap_chain_extent_.height); }
        VkFormat find_depth_format();

        VkResult acquire_next_image(uint32_t *image_index);
        VkResult submit_command_buffers(const VkCommandBuffer *buffers, uint32_t *image_index);

        bool compare_swap_formats(const SwapChain &swap_chain) const
        {
            return swap_chain_image_format_ == swap_chain.swap_chain_image_format_ &&
                swap_chain_depth_format_ == swap_chain.swap_chain_depth_format_;
        }

    private:
        void init();
        void create_swap_chain();
        void create_image_views();
        void create_depth_resources();
        void create_render_pass();
        void create_framebuffers();
        void create_sync_objects();

        VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats);
        VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes);
        VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities);

        VkFormat swap_chain_image_format_ = VK_FORMAT_UNDEFINED;
        VkFormat swap_chain_depth_format_ = VK_FORMAT_UNDEFINED;
        VkExtent2D swap_chain_extent_ = {0, 0};

        std::vector<VkFramebuffer> swap_chain_framebuffers_;
        VkRenderPass render_pass_ = VK_NULL_HANDLE;

        std::vector<VkImage> depth_images_;
        std::vector<VkDeviceMemory> depth_image_memories_;
        std::vector<VkImageView> depth_image_views_;
        std::vector<VkImage> swap_chain_images_;
        std::vector<VkImageView> swap_chain_image_views_;

        Device &device_;
        VkExtent2D window_extent_ = {0, 0};

        VkSwapchainKHR swap_chain_ = VK_NULL_HANDLE;
        std::shared_ptr<SwapChain> old_swap_chain_ = nullptr;

        std::vector<VkSemaphore> image_available_semaphores_;
        std::vector<VkSemaphore> render_finished_semaphores_;
        std::vector<VkFence> in_flight_fences_;
        std::vector<VkFence> images_in_flight_;
        size_t current_frame_ = 0;
    };
} // namespace xel::backend
