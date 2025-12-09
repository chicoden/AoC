#pragma once

#include <iostream>
#include <cstdint>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK(result) { \
    VkResult _result = result; \
    if (_result != VK_SUCCESS) { \
        std::cout << "error on line " << __LINE__ << ": " << string_VkResult(_result) << std::endl; \
        return 0; \
    } \
}

std::tuple<VkInstance, VkResult> create_vulkan_instance(
    const char* app_name,
    uint32_t app_version,
    uint32_t vk_api_version,
    const std::vector<const char*>& enabled_layers,
    const std::vector<const char*>& enabled_extensions
);
VkPhysicalDevice pick_physical_device(VkInstance instance, uint32_t score_gpu(VkPhysicalDevice gpu));
std::tuple<VkDevice, VkResult> create_logical_device(
    VkPhysicalDevice gpu,
    const std::vector<VkDeviceQueueCreateInfo>& queue_create_infos,
    const VkPhysicalDeviceFeatures2& enabled_features,
    const std::vector<const char*>& enabled_extensions
);
std::tuple<VkShaderModule, VkResult> create_shader_module_from_file(VkDevice device, const char* path);
VkDeviceAddress get_buffer_device_address(VkDevice device, VkBuffer buffer);
std::tuple<VkPipelineLayout, VkResult> create_pipeline_layout(
    VkDevice device,
    const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts,
    const std::vector<VkPushConstantRange>& push_constant_ranges
);
std::tuple<VkPipeline, VkResult> create_compute_pipeline(
    VkDevice device,
    VkPipelineLayout pipeline_layout,
    VkShaderModule shader_module,
    const char* entrypoint,
    const VkSpecializationInfo* specialization_info
);
