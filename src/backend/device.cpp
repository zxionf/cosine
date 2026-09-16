#include "device.hpp"

#include <cstring>
#include <iostream>
#include <unordered_set>
#include <set>

namespace xel::backend
{

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data
    ){
        std::cerr << "validation layer: " << callback_data->pMessage << std::endl;
        return VK_FALSE;
    }

    VkResult create_debug_utils_messenger_ext(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* debug_messenger
    ){
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, create_info, allocator, debug_messenger);
        }else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    Device::Device(Window &window) : window_{window}
    {
        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_command_pool();
    }

    Device::~Device()
    {
    }

    void Device::create_instance()
    {
        if (enable_validation_layers && !check_validation_layer_support())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Xel Engine";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        auto extensions = get_required_extensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
        if (enable_validation_layers)
        {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();

            populate_debug_messenger_create_info(debug_create_info);
            create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
        }else
        {
            create_info.enabledLayerCount = 0;
            create_info.pNext = nullptr;
        }

        if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }

        // has_glfw_required_instance_extensions();
    }

    void Device::setup_debug_messenger()
    {
        if (!enable_validation_layers) return;
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
        populate_debug_messenger_create_info(debug_create_info);
        if (create_debug_utils_messenger_ext(instance_, &debug_create_info, nullptr, &debug_messenger_) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void Device::create_surface()
    {
        window_.create_window_surface(instance_, &surface_);
    }

    void Device::pick_physical_device()
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

        if (device_count == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::cout << "Device Count: " << device_count << std::endl;
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
        for (const auto &device : devices)
        {
            if (is_device_suitable(device))
            {
                physical_device_ = device;
                break;
            }
        }
        if (physical_device_ == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        vkGetPhysicalDeviceProperties(physical_device_, &properties_);
        std::cout << "Device Name: " << properties_.deviceName << std::endl;
    }

    void Device::create_logical_device()
    {
        QueueFamilyIndices indices = find_queue_families(physical_device_);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {indices.graphics_family, indices.present_family};

        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info = {};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features = {};
        device_features.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();

        create_info.pEnabledFeatures = &device_features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();

        if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device_, indices.graphics_family, 0, &graphics_queue_);
        vkGetDeviceQueue(device_, indices.present_family, 0, &present_queue_);
    }

    void Device::create_command_pool()
    {
        QueueFamilyIndices indices = find_physical_queue_families();

        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = indices.graphics_family;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    bool Device::check_validation_layer_support()
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char *layer_name : validation_layers)
        {
            bool layer_found = false;

            for (const auto &layer_properties : available_layers)
            {
                if (strcmp(layer_name, layer_properties.layerName) == 0)
                {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found)
            {
                return false;
            }
        }
        return true;
    }

    std::vector<const char *> Device::get_required_extensions()
    {
        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        if (enable_validation_layers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void Device::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info)
    {
        create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debug_callback;
        create_info.pUserData = nullptr;  // Optional
    }

    void Device::has_glfw_required_instance_extensions() {
        uint32_t extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

        std::cout << "available extensions:" << std::endl;
        std::unordered_set<std::string> available;
        for (const auto &extension : extensions)
        {
            std::cout << "\t" << extension.extensionName << std::endl;
            available.insert(extension.extensionName);
        }
        std::cout << "required extensions:" << std::endl;
        auto requiredExtensions = get_required_extensions();
        for (const auto &required : requiredExtensions) {
            std::cout << "\t" << required << std::endl;
            if (available.find(required) == available.end())
            {
                throw std::runtime_error("Missing required glfw extension");
            }
        }
    }

    bool Device::is_device_suitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = find_queue_families(device);

        bool extensions_supported = check_device_extension_support(device);

        bool swap_chain_adequate = false;
        if (extensions_supported)
        {
            SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
            swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.presentModes.empty();
        }
        VkPhysicalDeviceFeatures supported_features;
        vkGetPhysicalDeviceFeatures(device, &supported_features);

        return indices.is_complete() && extensions_supported && swap_chain_adequate && supported_features.samplerAnisotropy;
    }

    QueueFamilyIndices Device::find_queue_families(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto &queue_family : queue_families)
        {
            if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
                indices.graphics_family_has_value = true;
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
            if (queue_family.queueCount > 0 && present_support)
            {
                indices.present_family = i;
                indices.present_family_has_value = true;
            }
            if (indices.is_complete()) break;
            i++;
        }

        return indices;
    }

    bool Device::check_device_extension_support(VkPhysicalDevice device)
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
        for (const auto &extension : available_extensions)
        {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    SwapChainSupportDetails Device::query_swap_chain_support(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);

        if (format_count != 0)
        {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
        }
        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);

        if (present_mode_count != 0)
        {
            details.presentModes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.presentModes.data());
        }

        return details;
    }
}