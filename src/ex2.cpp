#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

int main()
{
    // create instance
    vk::Instance instance_;
    vk::InstanceCreateInfo create_info;
    vk::ApplicationInfo app_info;
    app_info.setApiVersion(VK_API_VERSION_1_3);
    create_info.setPApplicationInfo(&app_info);
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.setEnabledExtensionCount(glfw_extension_count);
    create_info.setPpEnabledExtensionNames(glfw_extensions);
    create_info.setEnabledLayerCount(0);

    instance_ = vk::createInstance(create_info);

    printf("glfw extenion count: %d\n", glfw_extension_count);

    for (uint32_t i = 0; i < glfw_extension_count; i++)
        printf("glfw required extension [%u]: %s\n", i, glfw_extensions[i]);

    auto devices = instance_.enumeratePhysicalDevices();
    for (auto &device: devices)
        std::cout<< device.getProperties().deviceName << std::endl;
}