#include "swap_chain.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace xel::backend
{
    SwapChain::SwapChain(Device &device, VkExtent2D window_extent)
    : device_{device}, window_extent_{window_extent}
    {
        init();
    }

    SwapChain::SwapChain(Device &device, VkExtent2D window_extent, std::shared_ptr<SwapChain> previous)
    : device_{device}, window_extent_{window_extent}, old_swap_chain_{previous}
    {
        init();
        old_swap_chain_ = nullptr;
    }

    void SwapChain::init()
    {
        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_depth_resources();
        create_framebuffers();
        create_sync_objects();
    }

    SwapChain::~SwapChain()
    {
        for (auto image_view : swap_chain_image_views_)
        {
            vkDestroyImageView(device_.device(), image_view, nullptr);
        }
        swap_chain_images_.clear();
        if (swap_chain_ != nullptr)
        {
            vkDestroySwapchainKHR(device_.device(), swap_chain_, nullptr);
            swap_chain_ = nullptr;
        }
        for (int i = 0; i < depth_images_.size(); i++)
        {
            vkDestroyImageView(device_.device(), depth_image_views_[i], nullptr);
            vkDestroyImage(device_.device(), depth_images_[i], nullptr);
            vkFreeMemory(device_.device(), depth_image_memories_[i], nullptr);
        }
        for (auto framebuffer : swap_chain_framebuffers_)
        {
            vkDestroyFramebuffer(device_.device(), framebuffer, nullptr);
        }
        vkDestroyRenderPass(device_.device(), render_pass_, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device_.device(), render_finished_semaphores_[i], nullptr);
            vkDestroySemaphore(device_.device(), image_available_semaphores_[i], nullptr);
            vkDestroyFence(device_.device(), in_flight_fences_[i], nullptr);
        }
    }

    VkResult SwapChain::acquire_next_image(uint32_t *image_index)
    {
        vkWaitForFences(device_.device(), 1, &in_flight_fences_[current_frame_], VK_TRUE, std::numeric_limits<uint64_t>::max());
        VkResult result = vkAcquireNextImageKHR(device_.device(), swap_chain_, std::numeric_limits<uint64_t>::max(), image_available_semaphores_[current_frame_], VK_NULL_HANDLE, image_index);
        return result;
    }

    VkResult SwapChain::submit_command_buffers(const VkCommandBuffer *buffers, uint32_t *image_index)
    {

    }
} // namespace xel::backend