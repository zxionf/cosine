#pragma once

#include "window.hpp"

#include <vector>
#include <optional>

namespace xel::backend
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        bool is_complete() { return graphics_family.has_value() && present_family.has_value(); }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    class VulkanContext
    {
    public:
        VulkanContext(Window& window);
        ~VulkanContext();

        VulkanContext(const VulkanContext &) = delete;
        VulkanContext& operator=(const VulkanContext &) = delete;
        VulkanContext(VulkanContext&&) = delete;
        VulkanContext& operator=(VulkanContext&&) = delete;

        VkInstance instance() { return instance_; }
        VkDebugUtilsMessengerEXT debug_messenger() { return debug_messenger_; }
        VkSurfaceKHR surface() { return surface_; }
        VkPhysicalDevice physical_device() { return physical_device_; }
        VkDevice device() { return device_; }
        VkQueue graphics_queue() { return graphics_queue_; }
        VkQueue present_queue() { return present_queue_; }

        QueueFamilyIndices find_physical_queue_families() { return find_queue_family(physical_device_); }
        SwapChainSupportDetails get_swap_chain_support() { return query_swap_chain_support(physical_device_); }

    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();

        bool check_validation_layer_support();
        std::vector<const char *> get_required_extensions();
        void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);

        bool is_device_suitable(VkPhysicalDevice device);
        QueueFamilyIndices find_queue_family(VkPhysicalDevice device);
        bool check_device_extension_support(VkPhysicalDevice device);
        SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);

        VkInstance instance_;
        VkDebugUtilsMessengerEXT debug_messenger_;
        VkSurfaceKHR surface_;

        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE; // instance销毁时隐式销毁
        VkDevice device_;
        VkQueue graphics_queue_; // device销毁时隐式销毁
        VkQueue present_queue_; // device销毁时隐式销毁

        Window& window_;

        const std::vector<const char *> validation_layers_ = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> device_extensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };
} // namespace xel::backend