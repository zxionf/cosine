#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
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
        VulkanContext(Window& window);
        ~VulkanContext();

        const auto& surface()  const  { return surface_; }
        auto&       physical_device() { return physical_device_; }
        auto&       device()          { return device_; }
        auto&       graphics_queue()  { return graphics_queue_; }
        uint32_t    queue_index()     { return queue_index_; }
        Window&     window()          { return window_; }

    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface(GLFWwindow* window);
        void pick_physical_device();
        void create_logical_device();

        std::vector<const char*> get_required_instnace_extensions();
        bool is_device_suitable(vk::raii::PhysicalDevice const& device);

        vk::raii::Context                context_;
        vk::raii::Instance               instance_          = nullptr;
        vk::raii::DebugUtilsMessengerEXT debug_messenger_   = nullptr;
        vk::raii::SurfaceKHR             surface_           = nullptr;
        vk::raii::PhysicalDevice         physical_device_   = nullptr;
        vk::raii::Device                 device_            = nullptr;
        vk::raii::Queue                  graphics_queue_    = nullptr;
        uint32_t queue_index_ = ~0;

        Window& window_;

        const std::vector<const char*> validation_layers = {"VK_LAYER_KHRONOS_validation"};
        std::vector<const char*> required_device_extension = {vk::KHRSwapchainExtensionName};
    };
} // namespace xel::backend::vk