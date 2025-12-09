#include <iostream>
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include "housekeeper.hpp"
#include "vk_utilities.hpp"

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

const uint32_t WORKGROUP_SIZE = 32;
const uint32_t OP_ADD = 0;
const uint32_t OP_MUL = 1;
const uint32_t OP_COMBINE_RESULTS = 2;

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

    auto [math_shader, math_shader_status] = create_shader_module_from_file(device, "shaders/cephalopod_math.spv");
    VK_CHECK(math_shader_status);
    DEFER(cleanup_math_shader, vkDestroyShaderModule(device, math_shader, nullptr));

    auto [megabuffer, megabuffer_status] = create_buffer(
        device,
        16,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        {queue_family_indices.compute.value()}
    );
    VK_CHECK(megabuffer_status);
    DEFER(cleanup_megabuffer, vkDestroyBuffer(device, megabuffer, nullptr));

    std::cout << megabuffer << std::endl;///

    return 0;
}

/*#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <string>
#include <string_view>
#include <optional>
#include <utility>
#include <algorithm>
#include <vulkan/vulkan.h>
#include "housekeeper.hpp"

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#include "vk_mem_alloc.h"

const uint32_t OP_ADD = 0;
const uint32_t OP_MUL = 1;
const uint32_t WORKGROUP_SIZE = 32;

struct QueueFamilyIndices {
    std::optional<uint32_t> compute;
};

struct PushConstants {
    VkDeviceAddress problems;
    VkDeviceAddress results;
    uint32_t problem_stride;
    uint32_t problem_count;
    uint32_t opcode;
};

//bool create_vulkan_instance(VkInstance& instance, const char* app_name);
//std::tuple<VkPhysicalDevice, QueueFamilyIndices> pick_physical_device(VkInstance instance);
//QueueFamilyIndices find_queue_family_indices(VkPhysicalDevice gpu);
//bool create_logical_device(VkPhysicalDevice gpu, const QueueFamilyIndices& queue_family_indices, VkDevice& device);
//bool create_vma_allocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, VmaAllocator& allocator);
//bool load_shader_spv(VkDevice device, VkShaderModule& shader_module, const char* path);
bool create_buffer(VmaAllocator allocator, size_t size, VkBuffer& buffer, VmaAllocation& allocation);
bool create_pipeline_layout(VkDevice device, VkPipelineLayout& pipeline_layout);
bool create_compute_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, VkShaderModule shader_module, VkPipeline& pipeline);
bool create_command_pool(VkDevice device, uint32_t queue_family_index, VkCommandPool& command_pool);
bool allocate_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBufferLevel level, VkCommandBuffer& command_buffer);
bool create_fence(VkDevice device, bool create_signalled, VkFence& fence);

int main(int argc, char* argv[]) {
    if (argc != 2) { // first argument is implicit (the path of the executable)
        std::cout << "expected one argument, the input file" << std::endl;
        return 0;
    }

    VkInstance instance;
    if (!create_vulkan_instance(instance, "AoC 2025 - Day 6 Part 1")) {
        std::cout << "failed to create instance" << std::endl;
        return 0;
    } Housekeeper cleanup_instance([instance]() {
        vkDestroyInstance(instance, NULL);
        std::cout << "destroyed instance" << std::endl;
    });

    auto [gpu, queue_family_indices] = pick_physical_device(instance);
    if (gpu == VK_NULL_HANDLE) {
        std::cout << "no suitable gpu found" << std::endl;
        return 0;
    }

    VkDevice device;
    if (!create_logical_device(gpu, queue_family_indices, device)) {
        std::cout << "failed to create logical device" << std::endl;
        return 0;
    } Housekeeper cleanup_device([device]() {
        vkDestroyDevice(device, NULL);
        std::cout << "destroyed logical device" << std::endl;
    });

    VkQueue compute_queue;
    vkGetDeviceQueue(device, queue_family_indices.compute.value(), 0, &compute_queue);

    VmaAllocator allocator;
    if (!create_vma_allocator(instance, gpu, device, allocator)) {
        std::cout << "failed to create vma allocator" << std::endl;
        return 0;
    } Housekeeper cleanup_allocator([allocator]() {
        vmaDestroyAllocator(allocator);
        std::cout << "destroyed vma allocator" << std::endl;
    });

    VkShaderModule math_shader;
    if (!load_shader_spv(device, math_shader, "shaders/cephalopod_math.spv")) {
        std::cout << "failed to load shader binary" << std::endl;
        return 0;
    } Housekeeper cleanup_math_shader([device, math_shader]() {
        vkDestroyShaderModule(device, math_shader, NULL);
        std::cout << "destroyed shader module" << std::endl;
    });

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cout << "failed to open input file" << std::endl;
        return 0;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input_file, line)) {
        lines.push_back(std::move(line));
    }

    std::string& ops_line = lines.back();
    std::string::iterator ops_end = std::remove(ops_line.begin(), ops_line.end(), ' ');
    std::string_view ops = std::string_view(ops_line.begin(), ops_end);

    uint32_t add_problem_count = std::count(ops.begin(), ops.end(), '+');
    uint32_t mul_problem_count = std::count(ops.begin(), ops.end(), '*');
    uint32_t total_problem_count = ops.size();
    uint32_t values_per_problem = lines.size() - 1; // numbers for each problem are arranged vertically

    // compute offsets for ranges of problems and results in the gpu buffer
    uint32_t add_problems_offset = 0;
    uint32_t mul_problems_offset = add_problems_offset + add_problem_count * sizeof(uint32_t);
    uint32_t add_results_offset = (mul_problems_offset + mul_problem_count * sizeof(uint32_t) + 7) / 8 * 8; // round up to an alignment of 8
    uint32_t mul_results_offset =   add_results_offset + add_problem_count * sizeof(uint64_t);
    uint32_t buffer_size =          mul_results_offset + mul_problem_count * sizeof(uint64_t);
    uint32_t final_result_offset = add_results_offset;

    VkBuffer buffer;
    VmaAllocation allocation;
    if (!create_buffer(allocator, buffer_size, buffer, allocation)) {
        std::cout << "failed to allocate buffer" << std::endl;
        return 0;
    } Housekeeper cleanup_buffer([allocator, buffer, allocation]() {
        vmaDestroyBuffer(allocator, buffer, allocation);
        std::cout << "destroyed buffer" << std::endl;
    });

    VkBufferDeviceAddressInfo bda_info{};
    bda_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bda_info.buffer = buffer;
    VkDeviceAddress buffer_address = vkGetBufferDeviceAddress(device, &bda_info);

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

    VkPipelineLayout pipeline_layout;
    if (!create_pipeline_layout(device, pipeline_layout)) {
        std::cout << "failed to create pipeline layout" << std::endl;
        return 0;
    } Housekeeper cleanup_pipeline_layout([device, pipeline_layout]() {
        vkDestroyPipelineLayout(device, pipeline_layout, NULL);
        std::cout << "destroyed pipeline layout" << std::endl;
    });

    VkPipeline pipeline;
    if (!create_compute_pipeline(device, pipeline_layout, math_shader, pipeline)) {
        std::cout << "failed to create compute pipeline" << std::endl;
        return 0;
    } Housekeeper cleanup_pipeline([device, pipeline]() {
        vkDestroyPipeline(device, pipeline, NULL);
        std::cout << "destroyed compute pipeline" << std::endl;
    });

    VkCommandPool command_pool;
    if (!create_command_pool(device, queue_family_indices.compute.value(), command_pool)) {
        std::cout << "failed to create command pool" << std::endl;
        return 0;
    } Housekeeper cleanup_command_pool([device, command_pool]() {
        vkDestroyCommandPool(device, command_pool, NULL);
        std::cout << "destroyed command pool" << std::endl;
    });

    VkCommandBuffer command_buffer;
    if (!allocate_command_buffer(device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, command_buffer)) {
        std::cout << "failed to allocate command buffer" << std::endl;
        return 0;
    } // automatically freed when the owning command pool is destroyed

    VkFence work_complete_fence;
    if (!create_fence(device, false, work_complete_fence)) {
        std::cout << "failed to create fence" << std::endl;
        return 0;
    } Housekeeper cleanup_work_complete_fence([device, work_complete_fence]() {
        vkDestroyFence(device, work_complete_fence, NULL);
        std::cout << "destroyed fence" << std::endl;
    });

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

bool create_vulkan_instance(VkInstance& instance, const char* app_name) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = NULL;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;

    return vkCreateInstance(&create_info, NULL, &instance) == VK_SUCCESS;
}

std::tuple<VkPhysicalDevice, QueueFamilyIndices> pick_physical_device(VkInstance instance) {
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(instance, &gpu_count, NULL); // query number of gpus first

    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data()); // then provide space to store their handles

    VkPhysicalDevice selected_gpu = VK_NULL_HANDLE;
    QueueFamilyIndices selected_queue_family_indices;
    uint32_t best_score = 0; // not suitable at all
    for (const auto& gpu : gpus) {
        uint32_t score = 0;

        VkPhysicalDeviceProperties gpu_properties;
        vkGetPhysicalDeviceProperties(gpu, &gpu_properties);

        VkPhysicalDeviceFeatures2 gpu_features;
        gpu_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        VkPhysicalDeviceBufferDeviceAddressFeatures gpu_bda_features;
        gpu_bda_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        gpu_features.pNext = &gpu_bda_features;
        gpu_bda_features.pNext = NULL;
        vkGetPhysicalDeviceFeatures2(gpu, &gpu_features);

        QueueFamilyIndices queue_family_indices = find_queue_family_indices(gpu);

        if ( // can only receive a nonzero score if it is even suitable
            queue_family_indices.compute.has_value() &&
            gpu_features.features.shaderInt64 == VK_TRUE &&
            gpu_bda_features.bufferDeviceAddress == VK_TRUE
        ) {
            score += 1; // suitable

            switch (gpu_properties.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
                    score += 1000;
                    break;
                }

                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
                    score += 100;
                    break;
                }
            }
        }

        if (score > best_score) {
            selected_gpu = gpu;
            selected_queue_family_indices = queue_family_indices;
            best_score = score;
        }
    }

    return std::make_tuple(selected_gpu, selected_queue_family_indices);
}

QueueFamilyIndices find_queue_family_indices(VkPhysicalDevice gpu) {
    QueueFamilyIndices queue_family_indices{};

    uint32_t property_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &property_count, NULL); // query number of queue families

    std::vector<VkQueueFamilyProperties> properties(property_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &property_count, properties.data()); // then provide space to store their properties

    for (uint32_t index = 0; index < property_count; index++) {
        const auto& property = properties[index];
        if (property.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queue_family_indices.compute = index;
            break;
        }
    }

    return queue_family_indices;
}

bool create_logical_device(VkPhysicalDevice gpu, const QueueFamilyIndices& queue_family_indices, VkDevice& device) {
    std::vector<uint32_t> unique_queue_family_indices = {queue_family_indices.compute.value()};

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(unique_queue_family_indices.size());
    float queue_priorities[] = {1.0f};
    for (uint32_t i = 0; i < unique_queue_family_indices.size(); i++) {
        auto& create_info = queue_create_infos[i];
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info.queueFamilyIndex = unique_queue_family_indices[i];
        create_info.queueCount = 1;
        create_info.pQueuePriorities = queue_priorities;
    }

    VkPhysicalDeviceFeatures2 enabled_features{};
    enabled_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enabled_features.features.shaderInt64 = VK_TRUE;
    VkPhysicalDeviceBufferDeviceAddressFeatures enabled_bda_features{};
    enabled_bda_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    enabled_bda_features.bufferDeviceAddress = VK_TRUE;
    enabled_features.pNext = &enabled_bda_features;
    enabled_bda_features.pNext = NULL;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_create_infos.size();
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = NULL;
    create_info.pNext = &enabled_features;

    return vkCreateDevice(gpu, &create_info, NULL, &device) == VK_SUCCESS;
}

bool create_vma_allocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, VmaAllocator& allocator) {
    VmaAllocatorCreateInfo create_info{};
    create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    create_info.instance = instance;
    create_info.physicalDevice = gpu;
    create_info.device = device;
    return vmaCreateAllocator(&create_info, &allocator) == VK_SUCCESS;
}

bool load_shader_spv(VkDevice device, VkShaderModule& shader_module, const char* path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return false;

    size_t file_size = file.tellg(); // opened with read pointer at the end so tellg() gives the size in bytes
    file.seekg(0); // return file pointer to the start

    std::vector<char> code(file_size);
    file.read(code.data(), file_size);

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<uint32_t*>(code.data());

    return vkCreateShaderModule(device, &create_info, NULL, &shader_module) == VK_SUCCESS;
}

bool create_buffer(VmaAllocator allocator, size_t size, VkBuffer& buffer, VmaAllocation& allocation) {
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return vmaCreateBuffer(allocator, &create_info, &alloc_info, &buffer, &allocation, NULL) == VK_SUCCESS;
}

bool create_pipeline_layout(VkDevice device, VkPipelineLayout& pipeline_layout) {
    VkPushConstantRange push_constants_range{};
    push_constants_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constants_range.offset = 0;
    push_constants_range.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pushConstantRangeCount = 1;
    create_info.pPushConstantRanges = &push_constants_range;

    return vkCreatePipelineLayout(device, &create_info, NULL, &pipeline_layout) == VK_SUCCESS;
}

bool create_compute_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, VkShaderModule shader_module, VkPipeline& pipeline) {
    VkComputePipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.layout = pipeline_layout;
    create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    create_info.stage.module = shader_module;
    create_info.stage.pName = "main";
    return vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline) == VK_SUCCESS;
}

bool create_command_pool(VkDevice device, uint32_t queue_family_index, VkCommandPool& command_pool) {
    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue_family_index;
    return vkCreateCommandPool(device, &create_info, NULL, &command_pool) == VK_SUCCESS;
}

bool allocate_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBufferLevel level, VkCommandBuffer& command_buffer) {
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = level;
    alloc_info.commandBufferCount = 1;
    return vkAllocateCommandBuffers(device, &alloc_info, &command_buffer) == VK_SUCCESS;
}

bool create_fence(VkDevice device, bool create_signalled, VkFence& fence) {
    VkFenceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (create_signalled) create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return vkCreateFence(device, &create_info, NULL, &fence) == VK_SUCCESS;
}
*/