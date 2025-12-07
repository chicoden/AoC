cmake_minimum_required(VERSION 4.1)
set(CMAKE_CXX_STANDARD 20)

project(CephalopodMath)
    find_package(Vulkan 1.3 REQUIRED)

    file(GLOB shader_sources "src/shaders/*.glsl.*")
    set(shader_binaries "")
    foreach(shader_source_in ${shader_sources})
        get_filename_component(shader_name ${shader_source_in} NAME_WE)
        set(shader_binary_out "${CMAKE_SOURCE_DIR}/src/shaders/${shader_name}.spv")

        add_custom_command(
            OUTPUT ${shader_binary_out}
            COMMAND glslc ${shader_source_in} -o ${shader_binary_out}
            DEPENDS ${shader_source_in}
            COMMENT "Compiling shaders..."
            VERBATIM
        )

        list(APPEND shader_binaries ${shader_binary_out})
    endforeach()
    add_custom_target(compile_shaders ALL
        DEPENDS ${shader_binaries}
    )

    include_directories(${Vulkan_INCLUDE_DIRS})
    add_executable(CephalopodMath src/main.cpp)
    add_dependencies(CephalopodMath compile_shaders)
    target_link_libraries(CephalopodMath ${Vulkan_LIBRARIES})
