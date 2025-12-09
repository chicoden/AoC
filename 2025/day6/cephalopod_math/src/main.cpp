#include <iostream>
#include <fstream>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>
#include "housekeeper.hpp"
#include "vk_utilities.hpp"
#include "struct_builder.hpp"

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#include "vk_mem_alloc.h"

struct PushConstants {
    VkDeviceAddress problems;
    VkDeviceAddress results;
    uint32_t problem_stride;
    uint32_t problem_count;
    uint32_t opcode;
};

struct Queues {
    VkQueue compute;
};

struct QueueFamilyIndices {
    inline static const float DEFAULT_QUEUE_PRIORITY = 1.0f;
    std::optional<uint32_t> compute;

    static QueueFamilyIndices find(VkPhysicalDevice gpu);
    bool is_complete() const;
    std::vector<VkDeviceQueueCreateInfo> make_queue_create_infos() const;
    Queues get_queues(VkDevice device) const;
};

uint32_t calculate_gpu_score(VkPhysicalDevice gpu);

const uint32_t WORKGROUP_SIZE = 32;
const uint32_t OP_ADD = 0;
const uint32_t OP_MUL = 1;
const uint32_t OP_COMBINE_RESULTS = 2;

int main(int argc, char* argv[]) {
    if (argc != 2) { // first argument is implicit (the path of the executable)
        std::cout << "expected one argument, the input file" << std::endl;
        return 0;
    }

    auto [instance, instance_status] = create_vulkan_instance(
        "AoC 2025 - Day 6 Part 1",
        VK_MAKE_API_VERSION(0, 1, 0, 0),
        VK_API_VERSION_1_3,
        {}, {}
    );
    VK_CHECK(instance_status);
    DEFER(cleanup_instance, vkDestroyInstance(instance, nullptr));

    VkPhysicalDevice gpu = pick_physical_device(instance, calculate_gpu_score);
    if (gpu == VK_NULL_HANDLE) {
        std::cout << "no suitable gpu found" << std::endl;
        return 0;
    }

    QueueFamilyIndices queue_family_indices = QueueFamilyIndices::find(gpu);
    if (!queue_family_indices.is_complete()) {
        std::cout << "unable to find all required queue families" << std::endl;
        return 0;
    }

    VkPhysicalDeviceBufferDeviceAddressFeatures enabled_bda_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = nullptr,
        .bufferDeviceAddress = VK_TRUE
    };

    VkPhysicalDeviceFeatures2 enabled_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &enabled_bda_features,
        .features = {
            .shaderInt64 = VK_TRUE
        }
    };

    auto [device, device_status] = create_logical_device(
        gpu,
        queue_family_indices.make_queue_create_infos(),
        enabled_features, {}
    );
    VK_CHECK(device_status);
    DEFER(cleanup_device, vkDestroyDevice(device, nullptr));
    Queues queues = queue_family_indices.get_queues(device);

    VmaAllocator allocator; {
        VmaAllocatorCreateInfo create_info{};
        create_info.vulkanApiVersion = VK_API_VERSION_1_3;
        create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        create_info.instance = instance;
        create_info.physicalDevice = gpu;
        create_info.device = device;
        VK_CHECK(vmaCreateAllocator(&create_info, &allocator));
    } DEFER(cleanup_allocator, vmaDestroyAllocator(allocator));

    auto [math_shader, math_shader_status] = create_shader_module_from_file(device, "shaders/cephalopod_math.spv");
    VK_CHECK(math_shader_status);
    DEFER(cleanup_math_shader, vkDestroyShaderModule(device, math_shader, nullptr));

    VkPushConstantRange push_constant_range{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstants)
    };
    auto [pipeline_layout, pipeline_layout_status] = create_pipeline_layout(device, {}, {push_constant_range});
    VK_CHECK(pipeline_layout_status);
    DEFER(cleanup_pipeline_layout, vkDestroyPipelineLayout(device, pipeline_layout, nullptr));

    auto [pipeline, pipeline_status] = create_compute_pipeline(device, pipeline_layout, math_shader, "main", nullptr);
    VK_CHECK(pipeline_status);
    DEFER(cleanup_pipeline, vkDestroyPipeline(device, pipeline, nullptr));

    auto [command_pool, command_pool_status] = create_command_pool(device, 0, queue_family_indices.compute.value());
    VK_CHECK(command_pool_status);
    DEFER(cleanup_command_pool, vkDestroyCommandPool(device, command_pool, nullptr)); // also frees any command buffers allocated from the pool

    auto [command_buffer, command_buffer_status] = allocate_command_buffer(device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VK_CHECK(command_buffer_status);
    // automatically freed when parent command pool is destroyed

    auto [work_done_fence, work_done_fence_status] = create_fence(device, false);
    VK_CHECK(work_done_fence_status);
    DEFER(cleanup_work_done_fence, vkDestroyFence(device, work_done_fence, nullptr));

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cout << "failed to open input file " << argv[1] << std::endl;
        return 0;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input_file, line)) {
        lines.push_back(std::move(line));
    }

    std::string& ops_line = lines.back();
    std::string::iterator ops_end = std::remove(ops_line.begin(), ops_line.end(), ' ');
    std::string_view ops(ops_line.begin(), ops_end);

    size_t add_problem_count = 0;
    size_t mul_problem_count = 0;
    for (char op : ops) {
        switch (op) {
            case '+': add_problem_count++; break;
            case '*': mul_problem_count++; break;
        }
    }

    size_t total_problem_count = ops.size();
    size_t values_per_problem = lines.size() - 1; // problems are arranged vertically, the last line tells the operation

    StructBuilder struct_builder;
    size_t add_problems_offset = struct_builder.add<uint32_t>(add_problem_count * values_per_problem);
    size_t mul_problems_offset = struct_builder.add<uint32_t>(mul_problem_count * values_per_problem);
    size_t add_results_offset = struct_builder.add<uint64_t>(add_problem_count);
    size_t mul_results_offset = struct_builder.add<uint64_t>(mul_problem_count);
    size_t total_data_size = struct_builder.total_size();

    VkBuffer buffer;
    VmaAllocation buffer_allocation;
    VkDeviceAddress buffer_address; {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = total_data_size;
        buffer_info.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &buffer_allocation, nullptr));
        buffer_address = get_buffer_device_address(device, buffer);
    } DEFER(cleanup_buffer, vmaDestroyBuffer(allocator, buffer, buffer_allocation));

    std::cout << "Allocated buffer: " << buffer_address << std::endl;

    return 0;
}

uint32_t calculate_gpu_score(VkPhysicalDevice gpu) {
    VkPhysicalDeviceFeatures2 features;
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceBufferDeviceAddressFeatures bda_features;
    bda_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

    features.pNext = &bda_features;
    bda_features.pNext = nullptr;
    vkGetPhysicalDeviceFeatures2(gpu, &features);

    if (
        features.features.shaderInt64 == VK_FALSE ||
        bda_features.bufferDeviceAddress == VK_FALSE
    ) { // required features
        return 0;
    }

    VkPhysicalDeviceProperties2 properties;
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = nullptr;
    vkGetPhysicalDeviceProperties2(gpu, &properties);

    switch (properties.properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 1000; // prefer discrete gpu
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 100; // integrated gpu might be ok
    }

    return 0;
}

QueueFamilyIndices QueueFamilyIndices::find(VkPhysicalDevice gpu) {
    uint32_t property_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(gpu, &property_count, nullptr);
    std::vector<VkQueueFamilyProperties2> properties(property_count, VkQueueFamilyProperties2{
        .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
        .pNext = nullptr
    });
    vkGetPhysicalDeviceQueueFamilyProperties2(gpu, &property_count, properties.data());

    QueueFamilyIndices queue_family_indices{};
    for (uint32_t index = 0; index < property_count; index++) {
        const VkQueueFamilyProperties& property = properties[index].queueFamilyProperties;
        if (property.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queue_family_indices.compute = index;
            break;
        }
    }

    return queue_family_indices;
}

bool QueueFamilyIndices::is_complete() const {
    return this->compute.has_value();
}

std::vector<VkDeviceQueueCreateInfo> QueueFamilyIndices::make_queue_create_infos() const {
    std::vector<VkDeviceQueueCreateInfo> create_infos;
    create_infos.emplace_back(
        /* sType = */ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* queueFamilyIndex = */ this->compute.value(),
        /* queueCount = */ 1,
        /* pQueuePriorities = */ &QueueFamilyIndices::DEFAULT_QUEUE_PRIORITY
    );
    return create_infos;
}

Queues QueueFamilyIndices::get_queues(VkDevice device) const {
    Queues queues;
    vkGetDeviceQueue(device, this->compute.value(), 0, &queues.compute);
    return queues;
}

/*
    {
        void* data;
        if (vmaMapMemory(allocator, allocation, &data) != VK_SUCCESS) {
            std::cout << "failed to map buffer" << std::endl;
            return 0;
        }

        // write problem data into mapped buffer
        uint32_t* values = reinterpret_cast<uint32_t*>(data);
        /*for (char op : ops) {
        }*/
        ///

/*        vmaUnmapMemory(allocator, allocation);
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        std::cout << "failed to begin command buffer" << std::endl;
        return 0;
    }

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    PushConstants push_constants;
    push_constants.problem_stride = values_per_problem * sizeof(uint32_t);

    push_constants.problems = buffer_address + add_problems_offset;
    push_constants.results = buffer_address + add_results_offset;
    push_constants.problem_count = add_problem_count;
    push_constants.opcode = OP_ADD;
    vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &push_constants);
    vkCmdDispatch(command_buffer, (add_problem_count + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE, 1, 1);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        std::cout << "failed to end command buffer" << std::endl;
        return 0;
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    if (vkQueueSubmit(compute_queue, 1, &submit_info, work_complete_fence) != VK_SUCCESS) {
        std::cout << "failed to submit command buffer" << std::endl;
        return 0;
    }

    if (vkWaitForFences(device, 1, &work_complete_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        std::cout << "failed to wait on fence" << std::endl;
        return 0;
    }

    uint64_t result;
    {
        void* data;
        if (vmaMapMemory(allocator, allocation, &data) != VK_SUCCESS) {
            std::cout << "failed to map buffer" << std::endl;
            return 0;
        }

        // read final result out from mapped buffer
        result = *reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(data) + final_result_offset);

        vmaUnmapMemory(allocator, allocation);
    }

    std::cout << "Result: " << result << std::endl;
    return 0;
}
*/