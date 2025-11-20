cmake_minimum_required(VERSION 3.14)
project(EnGeneTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find system OpenGL (works on both Windows and Linux)
find_package(OpenGL REQUIRED)

# Create test executables (only for files that exist)
add_executable(EnGeneTest main.cpp)
add_executable(SimpleTest simple_test.cpp)
add_executable(SpecularGlossTest specular_gloss_test.cpp)
add_executable(SkyboxTest skybox_test.cpp)
add_executable(ClipPlaneFogTest clip_plane_fog_test.cpp)
add_executable(DualCameraTest dual_camera_test.cpp)
add_executable(ProjectiveTextureTest projective_texture_test.cpp)
add_executable(FramebufferBasicTest framebuffer_basic_test.cpp)
add_executable(FramebufferPostprocessTest framebuffer_postprocess_test.cpp)
add_executable(StencilTest stencil_test.cpp)
add_executable(BlendTest blend_test.cpp)
add_executable(RenderStateComponentTest renderstate_component_test.cpp)
add_executable(DepthTest depth_test.cpp)

# Function to configure a test target
function(configure_test_target target_name)
    # Add EnGene include directory (from parent directory)
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/../core_gene/src"
        "${CMAKE_SOURCE_DIR}/.."  # For other_genes/ directory
    )

    # Add dependency include directories from CoreGene-deps submodule
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/../libs/glad/include"
        "${CMAKE_SOURCE_DIR}/../libs/glfw/include"
        "${CMAKE_SOURCE_DIR}/../libs/glm/include"
        "${CMAKE_SOURCE_DIR}/../libs/stb/include"
    )

    # Platform-specific GLFW linking
    if(WIN32)
        # Windows: Use pre-built GLFW from libs/glfw/lib-mingw-w64
        target_link_libraries(${target_name} PRIVATE
            "${CMAKE_SOURCE_DIR}/../libs/glfw/lib-mingw-w64/libglfw3.a"
            OpenGL::GL
        )
        
        # Add backtrace support on Windows
        target_include_directories(${target_name} PRIVATE
            "${CMAKE_SOURCE_DIR}/../libs/backtrace/include"
        )
        target_link_libraries(${target_name} PRIVATE
            "${CMAKE_SOURCE_DIR}/../libs/backtrace/lib/libbacktrace.a"
        )
    else()
        # Linux/macOS: Try to find system GLFW first, fallback to building from source
        find_package(glfw3 3.3 QUIET)
        
        if(glfw3_FOUND)
            # Use system GLFW
            target_link_libraries(${target_name} PRIVATE
                glfw
                OpenGL::GL
            )
        else()
            # Build GLFW from submodule source (if available)
            if(EXISTS "${CMAKE_SOURCE_DIR}/../libs/glfw/CMakeLists.txt")
                message(STATUS "Building GLFW from submodule source")
                add_subdirectory("${CMAKE_SOURCE_DIR}/../libs/glfw" "${CMAKE_BINARY_DIR}/glfw" EXCLUDE_FROM_ALL)
                target_link_libraries(${target_name} PRIVATE
                    glfw
                    OpenGL::GL
                )
            else()
                message(FATAL_ERROR "GLFW not found. Please install GLFW3 or ensure the CoreGene-deps submodule is initialized.")
            endif()
        endif()
    endif()

    # Add debug symbols
    target_compile_options(${target_name} PRIVATE -g)
endfunction()

# Configure all test targets
configure_test_target(EnGeneTest)
configure_test_target(SimpleTest)
configure_test_target(SpecularGlossTest)
configure_test_target(SkyboxTest)
configure_test_target(ClipPlaneFogTest)
configure_test_target(DualCameraTest)
configure_test_target(ProjectiveTextureTest)
configure_test_target(FramebufferBasicTest)
configure_test_target(FramebufferPostprocessTest)
configure_test_target(StencilTest)
configure_test_target(BlendTest)
configure_test_target(RenderStateComponentTest)
configure_test_target(DepthTest)
