# EnGene Environment Test

This directory contains test projects to verify the EnGene environment configuration as documented in AGENTS.md.

## Test Structure

- `main.cpp` - Full test with custom GLSL shaders demonstrating advanced usage
- `simple_test.cpp` - Minimal test using default shaders for quick verification
- `CMakeLists.txt` - Build configuration supporting multiple test executables
- Build artifacts are placed in `../build/` directory

## What This Test Verifies

✅ **Header-Only Library Integration**
- EnGene.h includes correctly
- No .cpp files needed for the library
- GLAD_GL_IMPLEMENTATION is automatically defined

✅ **Dependency Configuration**
- GLFW library linking (MinGW-w64)
- GLAD OpenGL loader
- GLM mathematics library
- STB image library
- Backtrace library (optional debug)

✅ **Core API Functionality**
- EnGene application initialization
- EnGeneConfig structure usage
- Scene graph creation
- Component system (GeometryComponent)
- Geometry creation with interleaved vertex data
- Fixed timestep loop
- Rendering callback system

✅ **Build System**
- CMake configuration (minimum 3.14)
- MinGW Makefiles generator
- C++17 standard
- Debug symbols (-g flag)

## Build Instructions

From the repository root:

```bash
# Clean previous build (if needed)
Remove-Item -Recurse -Force build

# Configure the build
cmake -B build -S test -G "MinGW Makefiles"

# Compile both tests
cmake --build build

# Run the tests
.\build\EnGeneTest.exe      # Full test with custom shaders
.\build\SimpleTest.exe      # Simple test with default shaders
```

## Expected Behavior

When run successfully, the test application will:
1. Print initialization messages to console
2. Create an 800x600 window titled "EnGene Environment Test"
3. Display a colored triangle (red top, green bottom-left, blue bottom-right)
4. Run until the user closes the window or presses ESC

## Test Results

✅ **Build Status:** PASSED
- CMake configuration successful
- Compilation successful with MinGW-w64 GCC 15.2.0
- Minor warnings from library code (incomplete type in light_manager.h) - not test-related

✅ **API Verification:** PASSED
- Correct config field names: `title`, `width`, `height`, `clearColor`
- Correct exception type: `exception::EnGeneException`
- Correct namespace structure: `geometry::`, `scene::`, `component::`
- Scene node builder requires: `#include <core/scene_node_builder.h>`
- Components require: `#include <components/all.h>`

## Critical Pitfalls & Lessons Learned

### 1. **Missing Required Headers**

**Problem:** Forward declarations in `scene.h` cause compilation errors when using scene node builder.

**Solution:** Always include `<core/scene_node_builder.h>` when using the fluent builder API:
```cpp
#include <EnGene.h>
#include <core/scene_node_builder.h>  // REQUIRED for .addNode().with<>() syntax
#include <components/all.h>
```

### 2. **Custom Shader Uniform Configuration**

**Problem:** When using custom shaders, the `u_model` uniform is not automatically configured (only happens for default shaders).

**Solution:** Manually configure and re-bake the shader after EnGene construction:
```cpp
engene::EnGene app(on_initialize, on_fixed_update, on_render, config);

// CRITICAL: Configure u_model for custom shaders
auto base_shader = app.getBaseShader();
base_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
```

### 3. **Render Callback Must Clear Buffers**

**Problem:** Black screen even though geometry is created - buffers not cleared between frames.

**Solution:** Always call `glClear()` at the start of your render callback:
```cpp
auto on_render = [](double alpha) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // REQUIRED
    scene::graph()->draw();
    GL_CHECK("render");  // Optional but recommended
};
```

### 4. **GLSL Version Compatibility**

**Problem:** Using `#version 430` when default shaders use `#version 410`.

**Recommendation:** Match the OpenGL version specified in EnGene (4.3 Core) but use 410 for shader compatibility:
```glsl
#version 410 core  // Recommended for compatibility
```

### 5. **CMake Generator Selection**

**Problem:** Default generator (NMake) fails on MinGW-w64 systems.

**Solution:** Always specify MinGW Makefiles generator:
```bash
cmake -B build -S test -G "MinGW Makefiles"
```

## Best Practices

1. **Start with Default Shaders:** Use `simple_test.cpp` as a template - default shaders work out of the box
2. **Add Custom Shaders Incrementally:** Once default works, customize shaders one step at a time
3. **Always Include Error Checking:** Use `GL_CHECK()` macro after OpenGL calls in render loop
4. **Test in Separate Folder:** Keep test projects outside the library repository
5. **Use Raw String Literals:** Define shaders as `R"(...)"` for better readability and no escaping

## Notes

- This test follows the "separate test folder" approach recommended in AGENTS.md
- Test project is outside the core library to maintain header-only structure
- Build artifacts are isolated in the build/ directory
- No modifications to the core library were needed
- Minor warnings from `light_manager.h` are pre-existing library issues, not test-related
