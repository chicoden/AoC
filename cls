[1mdiff --git a/2025/day6/cephalopod_math/CMakeLists.txt b/2025/day6/cephalopod_math/CMakeLists.txt[m
[1mdeleted file mode 100644[m
[1mindex c79cffc..0000000[m
[1m--- a/2025/day6/cephalopod_math/CMakeLists.txt[m
[1m+++ /dev/null[m
[36m@@ -1,30 +0,0 @@[m
[31m-cmake_minimum_required(VERSION 4.1)[m
[31m-set(CMAKE_CXX_STANDARD 20)[m
[31m-[m
[31m-project(CephalopodMath)[m
[31m-    find_package(Vulkan 1.3 REQUIRED)[m
[31m-[m
[31m-    file(GLOB shader_sources "src/shaders/*.glsl.*")[m
[31m-    set(shader_binaries "")[m
[31m-    foreach(shader_source_in ${shader_sources})[m
[31m-        get_filename_component(shader_name ${shader_source_in} NAME_WE)[m
[31m-        set(shader_binary_out "${CMAKE_SOURCE_DIR}/src/shaders/${shader_name}.spv")[m
[31m-[m
[31m-        add_custom_command([m
[31m-            OUTPUT ${shader_binary_out}[m
[31m-            COMMAND glslc ${shader_source_in} -o ${shader_binary_out}[m
[31m-            DEPENDS ${shader_source_in}[m
[31m-            COMMENT "Compiling shaders..."[m
[31m-            VERBATIM[m
[31m-        )[m
[31m-[m
[31m-        list(APPEND shader_binaries ${shader_binary_out})[m
[31m-    endforeach()[m
[31m-    add_custom_target(compile_shaders ALL[m
[31m-        DEPENDS ${shader_binaries}[m
[31m-    )[m
[31m-[m
[31m-    include_directories(${Vulkan_INCLUDE_DIRS})[m
[31m-    add_executable(CephalopodMath src/main.cpp)[m
[31m-    add_dependencies(CephalopodMath compile_shaders)[m
[31m-    target_link_libraries(CephalopodMath ${Vulkan_LIBRARIES})[m
[1mdiff --git a/2025/day6/cephalopod_math/build_commands.bat b/2025/day6/cephalopod_math/build_commands.bat[m
[1mindex aba34c6..793c814 100644[m
[1m--- a/2025/day6/cephalopod_math/build_commands.bat[m
[1m+++ b/2025/day6/cephalopod_math/build_commands.bat[m
[36m@@ -1,4 +1,4 @@[m
[31m-cmake -B build -G "NMake Makefiles"[m
[32m+[m[32mcmake -S src -B build -G "NMake Makefiles"[m
 cd build[m
 nmake[m
 cd ..[m
[1mdiff --git a/2025/day6/cephalopod_math/src/main.cpp b/2025/day6/cephalopod_math/src/main.cpp[m
[1mindex 67912be..4a4cc3f 100644[m
[1m--- a/2025/day6/cephalopod_math/src/main.cpp[m
[1m+++ b/2025/day6/cephalopod_math/src/main.cpp[m
[36m@@ -15,6 +15,7 @@[m [mVkResult create_vulkan_instance(VkInstance& instance);[m
 std::tuple<VkPhysicalDevice, QueueFamilyIndices> pick_physical_device(VkInstance instance);[m
 QueueFamilyIndices find_queue_family_indices(VkPhysicalDevice gpu);[m
 VkResult create_logical_device(VkPhysicalDevice gpu, const QueueFamilyIndices& queue_family_indices, VkDevice& device);[m
[32m+[m[32mVkResult load_shader_spv(VkDevice device, VkShaderModule& shader_module, const char* path);[m
 [m
 int main(int argc, char* argv[]) {[m
     if (argc != 2) { // first argument is implicit (the path of the executable)[m
[36m@@ -45,7 +46,11 @@[m [mint main(int argc, char* argv[]) {[m
     VkQueue compute_queue;[m
     vkGetDeviceQueue(device, queue_family_indices.compute.value(), 0, &compute_queue);[m
 [m
[31m-    //[m
[32m+[m[32m    VkShaderModule cephalopod_math_shader;[m
[32m+[m[32m    if (load_shader_spv(device, cephalopod_math_shader, "shaders/cephalopod_math.spv") != VK_SUCCESS) {[m
[32m+[m[32m        std::cout << "failed to load shader module" << std::endl;[m
[32m+[m[32m        return 0;[m
[32m+[m[32m    }[m
 [m
     vkDestroyDevice(device, NULL);[m
     vkDestroyInstance(instance, NULL);[m
[36m@@ -162,3 +167,21 @@[m [mVkResult create_logical_device(VkPhysicalDevice gpu, const QueueFamilyIndices& q[m
 [m
     return vkCreateDevice(gpu, &create_info, NULL, &device);[m
 }[m
[32m+[m
[32m+[m[32mVkResult load_shader_spv(VkDevice device, VkShaderModule& shader_module, const char* path) {[m
[32m+[m[32m    std::ifstream file(path, std::ios::ate | std::ios::binary);[m
[32m+[m[32m    if (!file.is_open()) return VK_ERROR_UNKNOWN;[m
[32m+[m
[32m+[m[32m    size_t file_size = file.tellg(); // opened with read pointer at the end so tellg() gives the size in bytes[m
[32m+[m[32m    file.seekg(0); // return file pointer to the start[m
[32m+[m
[32m+[m[32m    std::vector<char> code(file_size);[m
[32m+[m[32m    file.read(code.data(), file_size);[m
[32m+[m
[32m+[m[32m    VkShaderModuleCreateInfo create_info{};[m
[32m+[m[32m    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;[m
[32m+[m[32m    create_info.codeSize = code.size();[m
[32m+[m[32m    create_info.pCode = reinterpret_cast<uint32_t*>(code.data());[m
[32m+[m
[32m+[m[32m    return vkCreateShaderModule(device, &create_info, NULL, &shader_module);[m
[32m+[m[32m}[m
[1mdiff --git a/2025/day6/cephalopod_math/vk_validation_log.txt b/2025/day6/cephalopod_math/vk_validation_log.txt[m
[1mindex c621f67..fa1c89a 100644[m
[1m--- a/2025/day6/cephalopod_math/vk_validation_log.txt[m
[1m+++ b/2025/day6/cephalopod_math/vk_validation_log.txt[m
[36m@@ -44,5 +44,5 @@[m [mLaunching Vulkan Application:[m
 - Working Directory: C:\Users\Pytho\Projects\AdventOfCode\2025\day6\cephalopod_math\build[m
 - Command-line Arguments: "C:\Users\Pytho\Projects\AdventOfCode\2025\day6\input.txt"[m
 - Log file: C:\Users\Pytho\Projects\AdventOfCode\2025\day6\cephalopod_math\vk_validation_log.txt[m
[31m-000001DDDEB1B040[m
[32m+[m[32mfailed to load shader module[m[41m[m
 Process terminated[m
\ No newline at end of file[m
