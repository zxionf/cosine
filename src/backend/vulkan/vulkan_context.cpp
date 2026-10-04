#include "vulkan_context.hpp"

#include <iostream>

namespace xel::backend::vulkan
{

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning || severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }

    VulkanContext::VulkanContext(Window& window) :window_{window}
    {
        create_instance();
        setup_debug_messenger();
        create_surface(window.get_glfw_window());
        pick_physical_device();
        create_logical_device();
    }

    VulkanContext::~VulkanContext()
    {
    }

    void VulkanContext::create_instance()
    {
        constexpr vk::ApplicationInfo app_info{
            .pApplicationName   = "Xel",
            .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
            .apiVersion         = vk::ApiVersion14
        };

        auto required_extensions = get_required_instnace_extensions();

        auto extension_properties = context_.enumerateInstanceExtensionProperties();
        auto unsupported_extension_it = std::ranges::find_if(required_extensions,
		                                               [&extension_properties](auto const &required_extension) {
			                                               return std::ranges::none_of(extension_properties,
                                                                [required_extension](auto const &extensionProperty) { return strcmp(extensionProperty.extensionName, required_extension) == 0; });
		                                               });
        if (unsupported_extension_it != required_extensions.end()) {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupported_extension_it));
        }

        std::vector<char const*> required_layers;
        if (enable_validation_layers) {
            required_layers.assign(validation_layers.begin(), validation_layers.end());
        }
        auto layer_properties = context_.enumerateInstanceLayerProperties();
        auto unsupported_layer_it = std::ranges::find_if(required_layers,
		                                               [&layer_properties](auto const &required_layer) {
			                                               return std::ranges::none_of(layer_properties,
                                                                [required_layer](auto const &layerProperty) { return strcmp(layerProperty.layerName, required_layer) == 0; });
		                                               });
        if (unsupported_layer_it != required_layers.end()) {
            throw std::runtime_error("Required layer not supported: " + std::string(*unsupported_layer_it));
        }

        vk::InstanceCreateInfo create_info{
            .pApplicationInfo       = &app_info,
            .enabledLayerCount      = static_cast<uint32_t>(required_layers.size()),
            .ppEnabledLayerNames    = required_layers.data(),
            .enabledExtensionCount  = static_cast<uint32_t>(required_extensions.size()),
            .ppEnabledExtensionNames = required_extensions.data()
        };

        instance_ = vk::raii::Instance{context_, create_info};
    }

    void VulkanContext::setup_debug_messenger()
    {
        if (!enable_validation_layers) return;
        std::cout << "enabled validation layers." << std::endl;
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                            .messageType     = messageTypeFlags,
                                                                            .pfnUserCallback = &debug_callback};
        debug_messenger_ = instance_.createDebugUtilsMessengerEXT( debugUtilsMessengerCreateInfoEXT );
    }

    void VulkanContext::create_surface(GLFWwindow* window)
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(*instance_, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        surface_ = vk::raii::SurfaceKHR{instance_, surface};
    }

    void VulkanContext::pick_physical_device()
    {
        auto physical_devices = instance_.enumeratePhysicalDevices();
        if (physical_devices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        auto const dev_iter = std::ranges::find_if(physical_devices, [&](auto const& device) {
            std::cout << "physical device: " << device.getProperties().deviceName << std::endl;
            return is_device_suitable(device);
        });
        if (dev_iter == physical_devices.end()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        physical_device_ = *dev_iter;
    }

    void VulkanContext::create_logical_device()
    {
        std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device_.getQueueFamilyProperties();

        uint32_t queue_index = ~0;
        for (uint32_t i = 0; i < queue_family_properties.size(); i++) {
            if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) && physical_device_.getSurfaceSupportKHR(i, *surface_)) {
                queue_index = i;
                break;
            }
        }
        if (queue_index == ~0) {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }

        queue_index_ = queue_index;

        // auto graphics_queue_family_property = std::ranges::find_if(queue_family_properties, [](auto const &qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
        // auto graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));

        vk::StructureChain<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        freatures_chain = {
            {},
            {.shaderDrawParameters = VK_TRUE},
            {.synchronization2 = true, .dynamicRendering = VK_TRUE},
            {.extendedDynamicState = VK_TRUE}
        };

        float queue_priority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex = queue_index, .queueCount = 1, .pQueuePriorities = &queue_priority };

        vk::DeviceCreateInfo device_create_info {
            .pNext                  = &freatures_chain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount   = 1,
            .pQueueCreateInfos      = &deviceQueueCreateInfo,
            .enabledExtensionCount  = static_cast<uint32_t>(required_device_extension.size()),
            .ppEnabledExtensionNames = required_device_extension.data()
        };

        device_         = vk::raii::Device{physical_device_, device_create_info};
        graphics_queue_ = vk::raii::Queue{device_, queue_index, 0};
    }

    bool VulkanContext::is_device_suitable(vk::raii::PhysicalDevice const& device) {
        bool support_vulkan1_3 = device.getProperties().apiVersion >= vk::ApiVersion13;

        auto queue_families = device.getQueueFamilyProperties();
        auto supports_graphics = std::ranges::any_of(queue_families, [](auto const& qfp) {
            return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
        });

        auto available_device_extensions = device.enumerateDeviceExtensionProperties();
        bool supports_all_required_extensions = std::ranges::all_of( required_device_extension, [&available_device_extensions]( auto const & required_device_extension ) {
                return std::ranges::any_of( available_device_extensions, [required_device_extension]( auto const & available_device_extension )
                    { return strcmp( available_device_extension.extensionName, required_device_extension ) == 0; } );
        });

        auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supports_required_features = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
        return support_vulkan1_3 && supports_graphics && supports_all_required_extensions && supports_required_features;
    }

    std::vector<const char*> VulkanContext::get_required_instnace_extensions()
    {
        uint32_t glfw_extension_count = 0;
        auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
        if (enable_validation_layers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }
} // namespace xel::backend::vulkan