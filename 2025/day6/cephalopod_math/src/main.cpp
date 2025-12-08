#include <iostream>
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

struct QueueFamilyIndices {
    std::optional<uint32_t> compute;
};

struct WorkData {
    VkDeviceMemory memory_backing;
    VkBuffer add_problems;
    VkBuffer mul_problems;
    VkBuffer summation_scratch;
    uint32_t add_problem_count;
    uint32_t mul_problem_count;
    uint32_t total_problem_count;
};

bool create_vulkan_instance(VkInstance& instance);
std::tuple<VkPhysicalDevice, QueueFamilyIndices> pick_physical_device(VkInstance instance);
QueueFamilyIndices find_queue_family_indices(VkPhysicalDevice gpu);
bool create_logical_device(VkPhysicalDevice gpu, const QueueFamilyIndices& queue_family_indices, VkDevice& device);
bool load_shader_spv(VkDevice device, VkShaderModule& shader_module, const char* path);

int main(int argc, char* argv[]) {
    if (argc != 2) { // first argument is implicit (the path of the executable)
        std::cout << "expected one argument, the input file" << std::endl;
        return 0;
    }

    VkInstance instance;
    if (!create_vulkan_instance(instance)) {
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

    VkShaderModule math_shader;
    if (!load_shader_spv(device, math_shader, "shaders/cephalopod_math.spv")) {
        std::cout << "failed to load shader binary" << std::endl;
        return 0;
    } Housekeeper cleanup_math_shader([device, math_shader]() {
        vkDestroyShaderModule(device, math_shader, NULL);
        std::cout << "destroyed shader module" << std::endl;
    });

    std::cout << math_shader << std::endl;///

    return 0;
}

bool create_vulkan_instance(VkInstance& instance) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "AoC 2025 - Day 6 Part 1";
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

        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(gpu, &device_properties);
        QueueFamilyIndices queue_family_indices = find_queue_family_indices(gpu);

        if ( // can only receive a nonzero score if it is even suitable
            queue_family_indices.compute.has_value()
        ) {
            score += 1; // suitable

            switch (device_properties.deviceType) {
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

    VkPhysicalDeviceFeatures enabled_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_create_infos.size();
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &enabled_features;
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = NULL;

    return vkCreateDevice(gpu, &create_info, NULL, &device) == VK_SUCCESS;
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

/*bool load_math_problems(VkDevice device, WorkData& work_data, const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(std::move(line));
    }

    std::string& ops_line = lines.back();
    const std::string::iterator ops_end = std::remove(ops_line.begin(), ops_line.end(), ' ');
    const std::string_view ops = std::string_view(ops_line.begin(), ops_end);
    std::cout << '"' << ops << '"' << std::endl;
}*/
