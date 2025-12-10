cmake_minimum_required(VERSION 4.1)
set(CMAKE_CXX_STANDARD 20)
set(ERRORS_AS_WARNINGS OFF CACHE BOOL "Enable errors as warnings for debugging, cannot always be enabled because VMA fucks things up")

if(${ERRORS_AS_WARNINGS})
    add_compile_options(/W4 /WX)
endif()

project(CephalopodMath)
    find_package(Vulkan 1.3 REQUIRED)

    file(GLOB shader_sources "shaders/*.glsl.*")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
    set(shader_binaries "")
    foreach(shader_source_in ${shader_sources})
        get_filename_component(shader_name ${shader_source_in} NAME_WE)
        set(shader_binary_out "${CMAKE_BINARY_DIR}/shaders/${shader_name}.spv")

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

    include_directories(${Vulkan_INCLUDE_DIRS} ../include)
    file(GLOB sources "*.cpp")
    add_executable(${CMAKE_PROJECT_NAME} ${sources})
    add_dependencies(${CMAKE_PROJECT_NAME} compile_shaders)
    target_link_libraries(${CMAKE_PROJECT_NAME} ${Vulkan_LIBRARIES})
