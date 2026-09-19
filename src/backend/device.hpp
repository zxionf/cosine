#pragma once

#include "window.hpp"

#include <vector>

namespace xel::backend
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices
    {
        uint32_t graphics_family;
        uint32_t present_family;
        bool graphics_family_has_value = false;
        bool present_family_has_value = false;
        bool is_complete() { return graphics_family_has_value && present_family_has_value; }
    };

    class Device
    {
    public:
#ifdef NDEBUG
        const bool enable_validation_layers = false;
#else
        const bool enable_validation_layers = true;
#endif

        Device(Window &window);
        ~Device();

        Device(const Device &) = delete;
        Device& operator=(const Device &) = delete;
        Device(Device &&) = delete;
        Device &operator=(Device &&) = delete;

        VkCommandPool get_command_pool() { return command_pool_; }
        VkDevice device() { return device_; }
        VkPhysicalDevice physical_device() { return physical_device_; }
        VkSurfaceKHR surface() { return surface_; }
        VkQueue graphics_queue() { return graphics_queue_; }
        VkQueue present_queue() { return present_queue_; }
        VkInstance instance() { return instance_; }
        VkDebugUtilsMessengerEXT debug_messenger() { return debug_messenger_; }

        SwapChainSupportDetails get_swap_chain_support() { return query_swap_chain_support(physical_device_); }
        QueueFamilyIndices find_physical_queue_families() { return find_queue_families(physical_device_); }
        uint32_t find_memory_type(uint32_t type_fliter, VkMemoryPropertyFlags properties);
        VkFormat find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);
        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
        void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count);
        void create_image_with_info(const VkImageCreateInfo &image_info, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory);

        VkPhysicalDeviceProperties properties_;
    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_command_pool();

        bool check_validation_layer_support();
        std::vector<const char *> get_required_extensions();
        void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info);
        void has_glfw_required_instance_extensions();
        bool is_device_suitable(VkPhysicalDevice device);
        QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
        bool check_device_extension_support(VkPhysicalDevice device);
        SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);

        VkInstance instance_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        Window &window_;
        VkDevice device_ = VK_NULL_HANDLE;
        VkQueue graphics_queue_ = VK_NULL_HANDLE;
        VkQueue present_queue_ = VK_NULL_HANDLE;

        const std::vector<const char *> validation_layers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char *> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
} // namespace xel::backend