# EnGene Environment Testing - Lessons Learned

**Date:** 2025-11-05  
**Tester:** Kiro AI Assistant  
**Environment:** Windows 10, MinGW-w64 GCC 15.2.0, CMake 3.14+

---

## Summary

Successfully tested and verified the EnGene environment configuration as documented in AGENTS.md. Discovered several critical pitfalls and documentation inaccuracies that have been corrected.

## Test Results

### ✅ What Works Correctly

1. **Header-Only Library Structure**
   - No compilation of library code needed
   - Include paths work as documented
   - GLAD_GL_IMPLEMENTATION automatically defined

2. **Dependency Linking**
   - GLFW, GLAD, GLM, STB, Backtrace all link correctly
   - MinGW-w64 static libraries work properly
   - OpenGL32.lib links without issues

3. **Core API Functionality**
   - Scene graph creation and traversal
   - Component system with priority-based execution
   - Geometry creation with interleaved vertex data
   - Transform stack and hierarchical transforms
   - Fixed timestep loop separation

4. **Build System**
   - CMake configuration works with MinGW Makefiles
   - Multiple test targets supported
   - Debug symbols (-g) work correctly

### ❌ Issues Discovered & Fixed

#### 1. Documentation Inaccuracies

**EnGeneConfig Field Names (CRITICAL)**
- **Documented (WRONG):** `window_title`, `window_width`, `window_height`, `clear_color`
- **Actual (CORRECT):** `title`, `width`, `height`, `clearColor[4]`
- **Impact:** Compilation errors for anyone following documentation
- **Fix:** Updated test code and documentation

**Exception Type Name**
- **Documented (WRONG):** `exception::BaseException`
- **Actual (CORRECT):** `exception::EnGeneException`
- **Impact:** Compilation errors in exception handling
- **Fix:** Updated AGENTS.md exception hierarchy section

#### 2. Missing Required Headers

**Problem:** Forward declarations in `scene.h` cause incomplete type errors when using scene node builder.

**Symptoms:**
```
error: invalid use of incomplete type 'class scene::SceneNodeBuilder'
```

**Solution:** Always include the builder header:
```cpp
#include <EnGene.h>
#include <core/scene_node_builder.h>  // REQUIRED
#include <components/all.h>
```

**Root Cause:** `scene.h` only forward-declares `SceneNodeBuilder`, actual definition is in `scene_node_builder.h`.

#### 3. Custom Shader Uniform Configuration

**Problem:** When using custom shaders, `u_model` uniform is not automatically configured (only happens for default shaders).

**Symptoms:**
```
Info: Active uniform 'u_model' (type: mat4)' is in the shader but not configured...
```

**Solution:** Manually configure after EnGene construction:
```cpp
engene::EnGene app(on_init, on_update, on_render, config);
auto shader = app.getBaseShader();
shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
shader->Bake();  // Re-bake to apply
```

**Root Cause:** EnGene.h line 72 only configures `u_model` if using `DEFAULT_VERTEX_SHADER`.

#### 4. Missing Buffer Clear

**Problem:** Black screen even though geometry is created correctly.

**Symptoms:** Window opens with clear color but no geometry visible.

**Solution:** Always clear buffers in render callback:
```cpp
auto on_render = [](double alpha) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // REQUIRED
    scene::graph()->draw();
};
```

**Root Cause:** OpenGL doesn't automatically clear buffers between frames.

#### 5. Vertex Format Mismatch

**Problem:** Initial test used `vec2` positions but default shader expects `vec4`.

**Symptoms:** Compilation succeeds but rendering may fail or produce incorrect results.

**Solution:** Match vertex data to shader expectations:
```cpp
// Shader: layout (location = 0) in vec4 vertex;
std::vector<float> vertices = {
    // x, y, z, w (vec4)
    0.0f, 0.5f, 0.0f, 1.0f,
};
auto geom = geometry::Geometry::Make(
    vertices.data(), indices.data(),
    3, 3,
    4,     // 4 floats for position
    {4}    // 4 floats for color
);
```

**Root Cause:** Default shader uses `vec4` for positions, not `vec2` or `vec3`.

#### 6. CMake Generator Issues

**Problem:** Default CMake generator (NMake) fails on MinGW-w64 systems.

**Symptoms:**
```
CMake Error: CMAKE_C_COMPILER not set, after EnableLanguage
```

**Solution:** Always specify generator:
```bash
cmake -B build -S test -G "MinGW Makefiles"
```

**Root Cause:** CMake defaults to Visual Studio generators on Windows, which don't work with MinGW.

---

## Recommendations for Documentation

### High Priority Updates

1. **Add Quick Troubleshooting Section** ✅ DONE
   - Common compilation errors and solutions
   - Black screen debugging steps
   - Build system issues

2. **Fix EnGeneConfig Documentation** ✅ DONE
   - Correct field names throughout
   - Show actual struct definition
   - Provide complete example

3. **Add Common Integration Pitfalls Section** ✅ DONE
   - Real code examples of wrong vs. right
   - Symptoms and solutions
   - Root cause explanations

4. **Update Exception Hierarchy** ✅ DONE
   - Remove non-existent `BaseException`
   - Clarify inheritance from `std::runtime_error`

### Medium Priority Updates

5. **Expand Testing Protocol Section**
   - Add note about separate test folders
   - Include CMake generator specification
   - Document expected warnings (light_manager.h)

6. **Add Minimal Working Example**
   - Simple triangle with default shaders
   - Custom shader variant
   - Both in test/ directory

7. **Document Render Callback Requirements**
   - Explicitly state `glClear()` is required
   - Show complete render callback template
   - Explain why it's needed

### Low Priority Enhancements

8. **Create Troubleshooting Flowchart**
   - Visual guide for common issues
   - Decision tree for debugging

9. **Add Video Tutorial Links**
   - Environment setup walkthrough
   - First triangle tutorial

10. **Expand Platform-Specific Notes**
    - More detailed MinGW setup
    - Linux-specific gotchas
    - macOS considerations

---

## Test Files Created

1. **`test/main.cpp`** - Full test with custom shaders
2. **`test/simple_test.cpp`** - Minimal test with defaults
3. **`test/CMakeLists.txt`** - Multi-target build configuration
4. **`test/README.md`** - Comprehensive test documentation
5. **`test/TESTING_NOTES.md`** - This file

---

## Verification Checklist

- [x] CMake configuration succeeds
- [x] Both test executables compile
- [x] No linker errors
- [x] Default shaders work (SimpleTest)
- [x] Custom shaders work (EnGeneTest)
- [x] Window opens with correct title and size
- [x] Clear color is applied
- [x] Geometry renders correctly
- [x] No OpenGL errors during runtime
- [x] Application closes cleanly
- [x] Documentation updated with findings

---

## Future Testing Recommendations

1. **Test on Other Platforms**
   - Linux with GCC/Clang
   - macOS with Clang
   - Windows with MSVC

2. **Test Different OpenGL Versions**
   - Verify 4.3 Core Profile requirement
   - Test fallback behavior

3. **Test More Complex Scenarios**
   - Multiple geometries
   - Texture loading
   - Lighting system
   - Camera movement

4. **Performance Testing**
   - Frame rate measurement
   - Memory leak detection (valgrind)
   - GPU profiling

5. **Integration Testing**
   - As git submodule
   - As system-wide installation
   - With different build systems (Make, Ninja)

---

## Conclusion

The EnGene environment configuration is fundamentally sound, but documentation had several critical inaccuracies that would prevent successful integration. All issues have been identified, documented, and fixed. The test suite provides working examples for both default and custom shader usage.

**Key Takeaway:** Always test documentation with a fresh environment to catch assumptions and outdated information.
