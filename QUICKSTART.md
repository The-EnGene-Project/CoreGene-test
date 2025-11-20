# EnGene Test Quick Start

Get up and running with EnGene in 5 minutes.

## Prerequisites

- Windows with MinGW-w64 GCC installed
- CMake 3.14 or higher
- Git (for cloning)

## Step 1: Clone and Setup

```bash
# Clone the repository
git clone <repository-url>
cd EnGene

# Verify dependencies are present
ls libs/  # Should see: glad, glfw, glm, stb, backtrace
```

## Step 2: Build the Simple Test

```bash
# Clean any previous builds
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# Configure with MinGW Makefiles
cmake -B build -S test -G "MinGW Makefiles"

# Build
cmake --build build
```

## Step 3: Run

```bash
# Run the simple test (default shaders)
.\build\SimpleTest.exe

# Or run the full test (custom shaders)
.\build\EnGeneTest.exe
```

## Expected Output

You should see:
1. Console messages about initialization
2. An 800x600 window opens
3. A colored triangle (red top, green/blue bottom corners)
4. Window closes when you press ESC or close it

## Troubleshooting

### "Incomplete type SceneNodeBuilder"
**Fix:** Add `#include <core/scene_node_builder.h>` to your code

### "BaseException not found"
**Fix:** Use `exception::EnGeneException` instead

### Black screen but window opens
**Fix:** Add `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);` in render callback

### CMake generator error
**Fix:** Use `cmake -B build -S test -G "MinGW Makefiles"`

### "window_title not a member"
**Fix:** Use `config.title` not `config.window_title`

## Next Steps

1. **Read the Code:** Check `simple_test.cpp` for minimal example
2. **Try Custom Shaders:** Look at `main.cpp` for advanced usage
3. **Read Documentation:** See `README.md` for detailed explanations
4. **Check Pitfalls:** Review `TESTING_NOTES.md` for common issues

## Minimal Code Template

```cpp
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>

int main() {
    auto on_init = [](engene::EnGene& app) {
        // Create geometry
        std::vector<float> vertices = { /* ... */ };
        std::vector<unsigned int> indices = { /* ... */ };
        auto geom = geometry::Geometry::Make(
            vertices.data(), indices.data(), 
            3, 3, 4, {4}
        );
        
        // Add to scene
        scene::graph()->addNode("MyNode")
            .with<component::GeometryComponent>(geom);
    };
    
    auto on_update = [](double dt) { };
    
    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene::graph()->draw();
    };
    
    engene::EnGeneConfig config;
    config.title = "My App";
    
    engene::EnGene app(on_init, on_update, on_render, config);
    app.run();
    
    return 0;
}
```

## File Structure

```
test/
├── QUICKSTART.md          ← You are here
├── README.md              ← Detailed documentation
├── TESTING_NOTES.md       ← Lessons learned
├── CHANGES_SUMMARY.md     ← What was fixed
├── CMakeLists.txt         ← Build configuration
├── simple_test.cpp        ← Minimal example
└── main.cpp               ← Full example with custom shaders
```

## Getting Help

1. Check `README.md` for detailed pitfalls
2. Review `TESTING_NOTES.md` for root causes
3. Look at working examples in `simple_test.cpp` and `main.cpp`
4. See AGENTS.md Quick Troubleshooting section

---

**Happy coding! 🚀**
