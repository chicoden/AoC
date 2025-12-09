#include <iostream>
#include <fstream>
#include <cstdint>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.h>
#include "vk_utilities.hpp"

std::tuple<VkInstance, VkResult> create_vulkan_instance(
    const char* app_name,
    uint32_t app_version,
    uint32_t vk_api_version,
    const std::vector<const char*>& enabled_layers,
    const std::vector<const char*>& enabled_extensions
) {
    VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = app_name,
        .applicationVersion = app_version,
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = vk_api_version
    };

    VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(enabled_layers.size()),
        .ppEnabledLayerNames = enabled_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size()),
        .ppEnabledExtensionNames = enabled_extensions.data()
    };

    VkInstance instance;
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
    return std::make_tuple(instance, result);
}

VkPhysicalDevice pick_physical_device(VkInstance instance, uint32_t score_gpu(VkPhysicalDevice gpu)) {
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr); // query number of gpus without allocating
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());

    VkPhysicalDevice best_gpu = VK_NULL_HANDLE;
    uint32_t best_score = 0; // not suitable
    for (VkPhysicalDevice gpu : gpus) {
        uint32_t score = score_gpu(gpu);
        if (score > best_score) {
            best_gpu = gpu;
            best_score = score;
        }
    }

    return best_gpu;
}

std::tuple<VkDevice, VkResult> create_logical_device(
    VkPhysicalDevice gpu,
    const std::vector<VkDeviceQueueCreateInfo> queue_create_infos,
    const VkPhysicalDeviceFeatures2& enabled_features,
    const std::vector<const char*>& enabled_extensions
) {
    VkDeviceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabled_features,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        //enabledLayerCount (legacy)
        //ppEnabledLayerNames (legacy)
        .enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size()),
        .ppEnabledExtensionNames = enabled_extensions.data(),
        //pEnabledFeatures (legacy)
    };

    VkDevice device;
    VkResult result = vkCreateDevice(gpu, &create_info, nullptr, &device);
    return std::make_tuple(device, result);
}

std::tuple<VkShaderModule, VkResult> create_shader_module_from_file(VkDevice device, const char* path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cout << "failed to open shader binary " << path << std::endl;
        return std::make_tuple(VK_NULL_HANDLE, VK_ERROR_UNKNOWN);
    }

    std::vector<char> code(file.tellg()); // opened with read pointer at end so tellg() gives the file size in bytes
    file.seekg(0); // return read pointer to start of file
    file.read(code.data(), code.size()); // read entire file

    VkShaderModuleCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<uint32_t*>(code.data())
    };

    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &shader_module);
    return std::make_tuple(shader_module, result);
}

std::tuple<VkBuffer, VkResult> create_buffer(
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const std::vector<uint32_t>& owning_queue_indices
) {
    VkBufferCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = owning_queue_indices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(owning_queue_indices.size()),
        .pQueueFamilyIndices = owning_queue_indices.data()
    };

    VkBuffer buffer;
    VkResult result = vkCreateBuffer(device, &create_info, nullptr, &buffer);
    return std::make_tuple(buffer, result);
}
