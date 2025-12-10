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
    const std::vector<VkDeviceQueueCreateInfo>& queue_create_infos,
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

VkDeviceAddress get_buffer_device_address(VkDevice device, VkBuffer buffer) {
    VkBufferDeviceAddressInfo bda_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = buffer
    };
    return vkGetBufferDeviceAddress(device, &bda_info);
}

std::tuple<VkPipelineLayout, VkResult> create_pipeline_layout(
    VkDevice device,
    const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts,
    const std::vector<VkPushConstantRange>& push_constant_ranges
) {
    VkPipelineLayoutCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size()),
        .pSetLayouts = descriptor_set_layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
        .pPushConstantRanges = push_constant_ranges.data()
    };

    VkPipelineLayout pipeline_layout;
    VkResult result = vkCreatePipelineLayout(device, &create_info, nullptr, &pipeline_layout);
    return std::make_tuple(pipeline_layout, result);
}

std::tuple<VkPipeline, VkResult> create_compute_pipeline(
    VkDevice device,
    VkPipelineLayout pipeline_layout,
    VkShaderModule shader_module,
    const char* entrypoint,
    const VkSpecializationInfo* specialization_info
) {
    VkComputePipelineCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module,
            .pName = entrypoint,
            .pSpecializationInfo = specialization_info
        },
        .layout = pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };

    VkPipeline pipeline;
    VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline);
    return std::make_tuple(pipeline, result);
}

std::tuple<VkCommandPool, VkResult> create_command_pool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queue_family_index) {
    VkCommandPoolCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queue_family_index
    };

    VkCommandPool command_pool;
    VkResult result = vkCreateCommandPool(device, &create_info, nullptr, &command_pool);
    return std::make_tuple(command_pool, result);
}

std::tuple<VkCommandBuffer, VkResult> allocate_command_buffer(
    VkDevice device,
    VkCommandPool parent_command_pool,
    VkCommandBufferLevel command_buffer_level
) {
    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = parent_command_pool,
        .level = command_buffer_level,
        .commandBufferCount = 1
    };

    VkCommandBuffer command_buffer;
    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);
    return std::make_tuple(command_buffer, result);
}

std::tuple<VkFence, VkResult> create_fence(VkDevice device, bool create_signalled) {
    VkFenceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = create_signalled ? VK_FENCE_CREATE_SIGNALED_BIT : (VkFenceCreateFlags)0
    };

    VkFence fence;
    VkResult result = vkCreateFence(device, &create_info, nullptr, &fence);
    return std::make_tuple(fence, result);
}

VkResult begin_command_buffer(
    VkCommandBuffer command_buffer,
    VkCommandBufferUsageFlags usage_flags,
    const VkCommandBufferInheritanceInfo* inheritance_info
) {
    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = usage_flags,
        .pInheritanceInfo = inheritance_info
    };

    return vkBeginCommandBuffer(command_buffer, &begin_info);
}

VkResult submit_command_buffer(VkQueue queue, VkFence fence) {
    VkSubmitInfo submit_info{
        .sType;
        .pNext;
        .waitSemaphoreCount;
        .pWaitSemaphores;
        .pWaitDstStageMask;
        .commandBufferCount;
        .pCommandBuffers;
        .signalSemaphoreCount;
        .pSignalSemaphores;
    };

    return vkQueueSubmit(queue, 1, &submit_info, fence);
}
