#include <iostream>
#include <fstream>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>
#include "housekeeper.hpp"
#include "vk_utilities.hpp"
#include "struct_builder.hpp"

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#include "vk_mem_alloc.h"

const uint32_t WORKGROUP_SIZE = 256;

enum Opcode : uint32_t {
    ADD = 0,
    MUL = 1,
    COMBINE_RESULTS = 2
};

struct PushConstants {
    uint64_t data_in_ptr;
    uint64_t data_out_ptr;
    uint32_t problem_count;
    uint32_t problem_stride;
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
void record_solve_math_problems_routine(
    VkCommandBuffer command_buffer,
    VkPipelineLayout pipeline_layout,
    VkDeviceAddress buffer_address,
    size_t problem_stride,
    size_t problem_count,
    size_t problems_offset,
    size_t results_offset,
    Opcode opcode
);
size_t record_sum_results_routine(
    VkCommandBuffer command_buffer,
    VkPipelineLayout pipeline_layout,
    VkDeviceAddress buffer_address,
    size_t result_count,
    size_t results_offset,
    size_t scratch_offset
);

int main(int argc, char* argv[]) {
    if (argc != 2) { // first argument is implicit (the path of the executable)
        std::cout << "expected one argument, the input file" << std::endl;
        return 0;
    }

    VkInstance instance;
    VK_CHECK(create_vulkan_instance(
        "AoC 2025 - Day 6 Part 1",
        VK_MAKE_API_VERSION(0, 1, 0, 0),
        VK_API_VERSION_1_3,
        {}, {},
        instance
    ));
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

    VkDevice device;
    VK_CHECK(create_logical_device(
        gpu,
        queue_family_indices.make_queue_create_infos(),
        enabled_features, {},
        device
    ));
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

    VkShaderModule math_shader;
    VK_CHECK(create_shader_module_from_file(device, "shaders/cephalopod_math.spv", math_shader));
    DEFER(cleanup_math_shader, vkDestroyShaderModule(device, math_shader, nullptr));

    VkPushConstantRange push_constant_range{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstants)
    };
    VkPipelineLayout pipeline_layout;
    VK_CHECK(create_pipeline_layout(device, {}, {push_constant_range}, pipeline_layout));
    DEFER(cleanup_pipeline_layout, vkDestroyPipelineLayout(device, pipeline_layout, nullptr));

    VkPipeline pipeline;
    VK_CHECK(create_compute_pipeline(device, pipeline_layout, math_shader, "main", nullptr, pipeline));
    DEFER(cleanup_pipeline, vkDestroyPipeline(device, pipeline, nullptr));

    VkCommandPool command_pool;
    VK_CHECK(create_command_pool(device, 0, queue_family_indices.compute.value(), command_pool));
    DEFER(cleanup_command_pool, vkDestroyCommandPool(device, command_pool, nullptr)); // also frees any command buffers allocated from the pool

    VkCommandBuffer command_buffer;
    VK_CHECK(allocate_command_buffer(device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, command_buffer));
    // automatically freed when parent command pool is destroyed

    VkFence work_done_fence;
    VK_CHECK(create_fence(device, false, work_done_fence));
    DEFER(cleanup_work_done_fence, vkDestroyFence(device, work_done_fence, nullptr));

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cout << "failed to open input file " << argv[1] << std::endl;
        return 0;
    }

    std::vector<std::stringstream> lines;
    std::string last_line;
    std::getline(input_file, last_line);
    while (true) {
        std::string buffer_line;
        if (!std::getline(input_file, buffer_line)) break;
        lines.emplace_back(std::move(last_line));
        last_line = std::move(buffer_line);
    }

    std::string::iterator ops_end = std::remove(last_line.begin(), last_line.end(), ' ');
    std::string_view ops(last_line.begin(), ops_end);

    size_t add_problem_count = 0;
    size_t mul_problem_count = 0;
    for (char op : ops) {
        switch (op) {
            case '+': add_problem_count++; break;
            case '*': mul_problem_count++; break;
        }
    }

    size_t total_problem_count = ops.size();
    size_t values_per_problem = lines.size();
    size_t problem_stride = values_per_problem * sizeof(uint32_t);

    StructBuilder struct_builder;
    size_t add_problems_offset = struct_builder.add<uint32_t>(add_problem_count * values_per_problem);
    size_t mul_problems_offset = struct_builder.add<uint32_t>(mul_problem_count * values_per_problem);
    size_t add_results_offset = struct_builder.add<uint64_t>(add_problem_count);
    size_t mul_results_offset = struct_builder.add<uint64_t>(mul_problem_count);
    size_t scratch_offset = struct_builder.add<uint64_t>(total_problem_count);
    size_t total_data_size = struct_builder.total_size();
    const size_t& results_offset = add_results_offset;

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

    {
        void* mapped_buffer;
        VK_CHECK(vmaMapMemory(allocator, buffer_allocation, &mapped_buffer));
        DEFER(unmap_buffer, vmaUnmapMemory(allocator, buffer_allocation));

        uint32_t* add_problems = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(mapped_buffer) + add_problems_offset);
        uint32_t* mul_problems = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(mapped_buffer) + mul_problems_offset);
        for (char op : ops) {
            uint32_t* problem;
            switch (op) {
                case '+': {
                    problem = add_problems;
                    add_problems += values_per_problem;
                    break;
                }

                case '*': {
                    problem = mul_problems;
                    mul_problems += values_per_problem;
                    break;
                }
            }

            for (std::stringstream& line : lines) {
                line >> *(problem++);
            }
        }
    }

    VK_CHECK(begin_command_buffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr));
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        record_solve_math_problems_routine(
            command_buffer,
            pipeline_layout,
            buffer_address,
            problem_stride,
            add_problem_count,
            add_problems_offset,
            add_results_offset,
            Opcode::ADD
        );
        record_solve_math_problems_routine(
            command_buffer,
            pipeline_layout,
            buffer_address,
            problem_stride,
            mul_problem_count,
            mul_problems_offset,
            mul_results_offset,
            Opcode::MUL
        );

        size_t final_result_offset = record_sum_results_routine(
            command_buffer,
            pipeline_layout,
            buffer_address,
            total_problem_count,
            results_offset,
            scratch_offset
        );
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VK_CHECK(submit_command_buffer(queues.compute, command_buffer, {}, {}, {}, work_done_fence));
    VK_CHECK(vkWaitForFences(device, 1, &work_done_fence, VK_TRUE, UINT64_MAX));

    {
        void* mapped_buffer;
        VK_CHECK(vmaMapMemory(allocator, buffer_allocation, &mapped_buffer));
        DEFER(unmap_buffer, vmaUnmapMemory(allocator, buffer_allocation));

        //uint64_t final_result = *reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(mapped_buffer) + final_result_offset);
        //std::cout << "Result: " << final_result << std::endl;
        std::cout << results_offset << ", " << scratch_offset << ", " << final_result_offset << std::endl;///
        uint64_t* values = reinterpret_cast<uint64_t*>(reinterpret_cast<uintptr_t>(mapped_buffer) + final_result_offset);
        for (int i = 0; i < 16; i++) std::cout << "Value: " << values[i] << std::endl;
    }

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

void record_solve_math_problems_routine(
    VkCommandBuffer command_buffer,
    VkPipelineLayout pipeline_layout,
    VkDeviceAddress buffer_address,
    size_t problem_stride,
    size_t problem_count,
    size_t problems_offset,
    size_t results_offset,
    Opcode opcode
) {
    PushConstants push_constants{
        .data_in_ptr = buffer_address + problems_offset,
        .data_out_ptr = buffer_address + results_offset,
        .problem_count = static_cast<uint32_t>(problem_count),
        .problem_stride = static_cast<uint32_t>(problem_stride),
        .opcode = opcode
    };

    uint32_t workgroup_count = (push_constants.problem_count + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
    vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants), &push_constants);
    vkCmdDispatch(command_buffer, workgroup_count, 1, 1);
}

size_t record_sum_results_routine(
    VkCommandBuffer command_buffer,
    VkPipelineLayout pipeline_layout,
    VkDeviceAddress buffer_address,
    size_t result_count,
    size_t results_offset,
    size_t scratch_offset
) {
    PushConstants push_constants{
        .data_in_ptr = buffer_address + results_offset,
        .data_out_ptr = buffer_address + scratch_offset,
        .problem_count = static_cast<uint32_t>(result_count),
        .opcode = Opcode::COMBINE_RESULTS
    };

    while (push_constants.problem_count > 1) {
        ///vkCmdPipelineBarrier();

        uint32_t workgroup_count = (push_constants.problem_count + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
        vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants), &push_constants);
        vkCmdDispatch(command_buffer, workgroup_count, 1, 1);

        std::swap(push_constants.data_in_ptr, push_constants.data_out_ptr); // ping pong
        push_constants.problem_count = workgroup_count; // each workgroup reduces a block of results to a single result
    }

    return push_constants.data_in_ptr - buffer_address; // return offset to final result
}
