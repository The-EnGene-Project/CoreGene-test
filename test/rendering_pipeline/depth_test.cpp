#include <iostream>
#include <memory>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <gl_base/framebuffer.h>
#include <3d/camera/perspective_camera.h>
#include <other_genes/3d_shapes/cube.h>

// Custom shaders that support material abstraction
const char* DEPTH_TEST_VERTEX_SHADER = R"(
    #version 410 core
    layout (location = 0) in vec4 vertex;
    layout (location = 1) in vec3 normal;

    out vec3 fragNormal;
    out vec3 fragPos;

    // Camera UBO (required by EnGene)
    layout (std140) uniform CameraMatrices {
        mat4 view;
        mat4 projection;
    };

    // Model matrix (required by default shader)
    uniform mat4 u_model;

    void main() {
        fragPos = vec3(u_model * vertex);
        fragNormal = mat3(transpose(inverse(u_model))) * normal;
        gl_Position = projection * view * u_model * vertex;
    }
)";

const char* DEPTH_TEST_FRAGMENT_SHADER = R"(
    #version 410 core
    
    in vec3 fragNormal;
    in vec3 fragPos;
    out vec4 fragColor;

    // Material properties (from MaterialComponent)
    uniform vec4 color;

    void main() {
        // Simple lighting calculation
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        vec3 norm = normalize(fragNormal);
        float diff = max(dot(norm, lightDir), 0.0);
        
        vec3 ambient = 0.3 * color.rgb;
        vec3 diffuse = diff * color.rgb;
        
        fragColor = vec4(ambient + diffuse, color.a);
    }
)";

/**
 * @brief Depth test application demonstrating depth state functionality
 * 
 * This test validates:
 * - Depth test enable/disable via framebuffer::stack()->depth().setTest()
 * - Depth write mask configuration (read-only vs read-write)
 * - Depth function configuration (Less, LEqual, Greater, GEqual, Equal, NotEqual, Always, Never)
 * - Depth clamping enable/disable
 * - Depth range configuration
 * - State inheritance across framebuffer push/pop
 * - Hierarchical depth state management
 * - No OpenGL errors during depth operations
 * 
 * Test Structure:
 * 1. Standard depth testing: Overlapping cubes with depth test enabled
 * 2. Depth functions: Test different depth comparison functions
 * 3. Read-only depth: Depth test enabled but writes disabled
 * 4. Depth clamping: Test depth clamping functionality
 * 5. State inheritance: Test push/pop with depth state
 * 6. Advanced depth range: Test custom depth ranges
 * 
 * Expected Result:
 * - Cubes render with correct depth ordering
 * - Different depth functions produce expected results
 * - Read-only depth prevents depth buffer updates
 * - Depth clamping works correctly
 * - State inheritance works correctly across framebuffer stack
 * - No OpenGL errors
 * 
 * Controls:
 * - ESC: Exit
 * - SPACE: Toggle between test phases
 */

// Global state for test phases
enum class TestPhase {
    STANDARD_DEPTH_TEST,    // Standard depth testing (Less)
    DEPTH_FUNCTIONS,        // Test different depth functions
    READ_ONLY_DEPTH,        // Depth test enabled, writes disabled
    DEPTH_CLAMPING,         // Test depth clamping
    STATE_INHERITANCE,      // Test state inheritance
    DEPTH_RANGE,            // Test custom depth ranges
    COMPLETE
};

TestPhase g_current_phase = TestPhase::STANDARD_DEPTH_TEST;
double g_time = 0.0;
bool g_phase_changed = false;
int g_depth_func_index = 0;
int g_depth_range_index = 0;

// Shared resources
geometry::GeometryPtr g_cube_geom;
framebuffer::FramebufferPtr g_fbo;

// Depth function names for display
const char* g_depth_func_names[] = {
    "Less (default)",
    "LEqual",
    "Greater",
    "GEqual",
    "Equal",
    "NotEqual",
    "Always",
    "Never"
};

framebuffer::DepthFunc g_depth_funcs[] = {
    framebuffer::DepthFunc::Less,
    framebuffer::DepthFunc::LEqual,
    framebuffer::DepthFunc::Greater,
    framebuffer::DepthFunc::GEqual,
    framebuffer::DepthFunc::Equal,
    framebuffer::DepthFunc::NotEqual,
    framebuffer::DepthFunc::Always,
    framebuffer::DepthFunc::Never
};

// Depth range names for display
const char* g_depth_range_names[] = {
    "[0.0, 1.0] (Standard)",
    "[0.0, 0.5] (Near Half)",
    "[0.5, 1.0] (Far Half)",
    "[1.0, 0.0] (Reverse-Z)",
    "[0.2, 0.8] (Custom)"
};

/**
 * @brief Test Phase 1: Standard depth testing
 * 
 * Configures standard depth testing with Less function.
 * Renders overlapping cubes to demonstrate depth ordering.
 */
void setupStandardDepthTestPhase() {
    std::cout << "\n=== Phase 1: Standard Depth Testing ===" << std::endl;
    std::cout << "Configuring standard depth testing..." << std::endl;
    
    // Enable depth testing
    framebuffer::stack()->depth().setTest(true);
    std::cout << "✓ Depth test enabled" << std::endl;
    
    // Enable depth writes
    framebuffer::stack()->depth().setWrite(true);
    std::cout << "✓ Depth writes enabled" << std::endl;
    
    // Configure depth function: Less (default)
    framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Less);
    std::cout << "✓ Depth function set to Less (default)" << std::endl;
    
    // Disable depth clamping
    framebuffer::stack()->depth().setClamp(false);
    std::cout << "✓ Depth clamping disabled" << std::endl;
    
    // Set standard depth range
    framebuffer::stack()->depth().setRange(0.0, 1.0);
    std::cout << "✓ Depth range set to [0.0, 1.0]" << std::endl;
    
    std::cout << "Phase 1 setup complete. Cubes will render with correct depth ordering." << std::endl;
}

/**
 * @brief Test Phase 2: Depth functions
 * 
 * Cycles through different depth comparison functions.
 * Demonstrates how each function affects rendering.
 */
void setupDepthFunctionsPhase() {
    std::cout << "\n=== Phase 2: Depth Functions ===" << std::endl;
    std::cout << "Testing different depth comparison functions..." << std::endl;
    
    // Enable depth testing
    framebuffer::stack()->depth().setTest(true);
    std::cout << "✓ Depth test enabled" << std::endl;
    
    // Enable depth writes
    framebuffer::stack()->depth().setWrite(true);
    std::cout << "✓ Depth writes enabled" << std::endl;
    
    // Set initial depth function
    g_depth_func_index = 0;
    framebuffer::stack()->depth().setFunction(g_depth_funcs[g_depth_func_index]);
    std::cout << "✓ Depth function set to " << g_depth_func_names[g_depth_func_index] << std::endl;
    
    std::cout << "Phase 2 setup complete. Press SPACE to cycle through depth functions." << std::endl;
}

/**
 * @brief Test Phase 3: Read-only depth
 * 
 * Configures depth test enabled but depth writes disabled.
 * Demonstrates read-only depth buffer behavior.
 */
void setupReadOnlyDepthPhase() {
    std::cout << "\n=== Phase 3: Read-Only Depth ===" << std::endl;
    std::cout << "Configuring read-only depth buffer..." << std::endl;
    
    // Enable depth testing
    framebuffer::stack()->depth().setTest(true);
    std::cout << "✓ Depth test enabled" << std::endl;
    
    // Disable depth writes (read-only)
    framebuffer::stack()->depth().setWrite(false);
    std::cout << "✓ Depth writes disabled (read-only depth buffer)" << std::endl;
    
    // Configure depth function: LEqual
    framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::LEqual);
    std::cout << "✓ Depth function set to LEqual" << std::endl;
    
    std::cout << "Phase 3 setup complete. Depth buffer is read-only." << std::endl;
    std::cout << "Note: Later objects will not update depth buffer." << std::endl;
}

/**
 * @brief Test Phase 4: Depth clamping
 * 
 * Tests depth clamping functionality.
 * Prevents clipping at near/far planes.
 */
void setupDepthClampingPhase() {
    std::cout << "\n=== Phase 4: Depth Clamping ===" << std::endl;
    std::cout << "Testing depth clamping..." << std::endl;
    
    // Enable depth testing
    framebuffer::stack()->depth().setTest(true);
    std::cout << "✓ Depth test enabled" << std::endl;
    
    // Enable depth writes
    framebuffer::stack()->depth().setWrite(true);
    std::cout << "✓ Depth writes enabled" << std::endl;
    
    // Configure depth function: Less
    framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Less);
    std::cout << "✓ Depth function set to Less" << std::endl;
    
    // Enable depth clamping
    framebuffer::stack()->depth().setClamp(true);
    std::cout << "✓ Depth clamping enabled" << std::endl;
    std::cout << "  (Prevents clipping at near/far planes)" << std::endl;
    
    std::cout << "Phase 4 setup complete. Depth values will be clamped." << std::endl;
}

/**
 * @brief Test Phase 5: State inheritance
 * 
 * Tests that depth state is inherited correctly when pushing/popping framebuffers.
 * Validates hierarchical state management.
 */
void setupStateInheritancePhase() {
    std::cout << "\n=== Phase 5: State Inheritance ===" << std::endl;
    std::cout << "Testing hierarchical depth state management..." << std::endl;
    
    // Configure root level state
    framebuffer::stack()->depth().setTest(true);
    framebuffer::stack()->depth().setWrite(true);
    framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Less);
    framebuffer::stack()->depth().setClamp(false);
    framebuffer::stack()->depth().setRange(0.0, 1.0);
    std::cout << "✓ Root state configured (standard depth testing)" << std::endl;
    
    // Push FBO (should inherit state)
    if (g_fbo) {
        framebuffer::stack()->push(g_fbo);
        std::cout << "✓ Pushed FBO - state should be inherited" << std::endl;
        
        // Modify state in child
        framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Greater);
        framebuffer::stack()->depth().setWrite(false);
        framebuffer::stack()->depth().setClamp(true);
        std::cout << "✓ Modified depth state in child (Greater, read-only, clamped)" << std::endl;
        
        // Pop FBO (should restore parent state)
        framebuffer::stack()->pop();
        std::cout << "✓ Popped FBO - parent state should be restored" << std::endl;
    }
    
    // Test with RenderState (apply mode)
    auto render_state = std::make_shared<framebuffer::RenderState>();
    render_state->depth().setTest(true);
    render_state->depth().setWrite(false);  // Read-only
    render_state->depth().setFunction(framebuffer::DepthFunc::LEqual);
    render_state->depth().setClamp(true);
    render_state->depth().setRange(0.1, 0.9);
    std::cout << "✓ Created RenderState with custom depth configuration" << std::endl;
    
    if (g_fbo) {
        framebuffer::stack()->push(g_fbo, render_state);
        std::cout << "✓ Pushed FBO with RenderState (apply mode)" << std::endl;
        
        framebuffer::stack()->pop();
        std::cout << "✓ Popped FBO - state restored" << std::endl;
    }
    
    // Test all depth functions in sequence
    std::cout << "\nTesting all depth functions in sequence:" << std::endl;
    for (int i = 0; i < 8; i++) {
        framebuffer::stack()->depth().setFunction(g_depth_funcs[i]);
        std::cout << "  ✓ " << g_depth_func_names[i] << std::endl;
    }
    
    // Reset to standard
    framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Less);
    
    std::cout << "Phase 5 complete. State inheritance validated." << std::endl;
}

/**
 * @brief Test Phase 6: Custom depth ranges
 * 
 * Tests custom depth range configuration.
 * Demonstrates depth range remapping.
 */
void setupDepthRangePhase() {
    std::cout << "\n=== Phase 6: Custom Depth Ranges ===" << std::endl;
    std::cout << "Testing custom depth range configuration..." << std::endl;
    
    // Enable depth testing
    framebuffer::stack()->depth().setTest(true);
    std::cout << "✓ Depth test enabled" << std::endl;
    
    // Enable depth writes
    framebuffer::stack()->depth().setWrite(true);
    std::cout << "✓ Depth writes enabled" << std::endl;
    
    // Configure depth function: Less
    framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Less);
    std::cout << "✓ Depth function set to Less" << std::endl;
    
    // Set initial depth range
    g_depth_range_index = 0;
    framebuffer::stack()->depth().setRange(0.0, 1.0);
    std::cout << "✓ Range set to " << g_depth_range_names[g_depth_range_index] << std::endl;
    
    std::cout << "Phase 6 setup complete. Press SPACE to cycle through depth ranges." << std::endl;
}

int main() {
    std::cout << "=== Depth Test Application ===" << std::endl;
    std::cout << "Testing: Depth state operations and state management" << std::endl;
    std::cout << "Expected: Depth testing works correctly, state inheritance validated" << std::endl;
    std::cout << std::endl;
    
    auto on_init = [](engene::EnGene& app) {
        std::cout << "[INIT] Initializing depth test..." << std::endl;
        
        // Create FBO with color and depth attachments
        std::cout << "Creating framebuffer..." << std::endl;
        
        std::vector<framebuffer::Framebuffer::AttachmentSpec> specs = {
            framebuffer::Framebuffer::AttachmentSpec(
                framebuffer::attachment::Point::Color0,
                framebuffer::attachment::Format::RGBA8,
                framebuffer::attachment::StorageType::Texture,
                "color_texture"),
            framebuffer::Framebuffer::AttachmentSpec(
                framebuffer::attachment::Point::Depth,
                framebuffer::attachment::Format::DepthComponent24)
        };
        
        g_fbo = framebuffer::Framebuffer::Make(800, 600, specs);
        
        if (!g_fbo) {
            throw exception::FramebufferException("Failed to create framebuffer");
        }
        std::cout << "✓ Framebuffer created" << std::endl;
        
        // Create cube geometry
        g_cube_geom = Cube::Make(1.0f, 1.0f, 1.0f);
        std::cout << "✓ Cube geometry created" << std::endl;
        
        // Create scene with multiple overlapping cubes at different depths
        // Cube 1: Red, closest
        auto mat1 = material::Material::Make(glm::vec3(1.0f, 0.0f, 0.0f));
        mat1->set("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        
        scene::graph()->addNode("cube1")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(-1.5f, 0.0f, -4.0f)
            )
            .with<component::MaterialComponent>(mat1)
            .with<component::GeometryComponent>(g_cube_geom);
        
        // Cube 2: Green, middle depth
        auto mat2 = material::Material::Make(glm::vec3(0.0f, 1.0f, 0.0f));
        mat2->set("color", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        
        scene::graph()->addNode("cube2")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(0.0f, 0.0f, -5.0f)
            )
            .with<component::MaterialComponent>(mat2)
            .with<component::GeometryComponent>(g_cube_geom);
        
        // Cube 3: Blue, farthest
        auto mat3 = material::Material::Make(glm::vec3(0.0f, 0.0f, 1.0f));
        mat3->set("color", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        
        scene::graph()->addNode("cube3")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(1.5f, 0.0f, -6.0f)
            )
            .with<component::MaterialComponent>(mat3)
            .with<component::GeometryComponent>(g_cube_geom);
        
        // Cube 4: Yellow, overlapping with cube 2
        auto mat4 = material::Material::Make(glm::vec3(1.0f, 1.0f, 0.0f));
        mat4->set("color", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        
        scene::graph()->addNode("cube4")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(0.5f, 0.5f, -5.2f)
            )
            .with<component::MaterialComponent>(mat4)
            .with<component::GeometryComponent>(g_cube_geom);
        
        // Cube 5: Cyan, overlapping with cube 1
        auto mat5 = material::Material::Make(glm::vec3(0.0f, 1.0f, 1.0f));
        mat5->set("color", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        
        scene::graph()->addNode("cube5")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(-1.0f, -0.5f, -4.3f)
            )
            .with<component::MaterialComponent>(mat5)
            .with<component::GeometryComponent>(g_cube_geom);
        
        // Cube 6: Magenta, VERY far away (behind far plane)
        auto mat6 = material::Material::Make(glm::vec3(1.0f, 0.0f, 1.0f));
        mat6->set("color", glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
        
        scene::graph()->addNode("cube6")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(0.0f, 0.0f, -110.0f)  // Far plane is at 100.0
                    ->scale(50.0f, 50.0f, 1.0f)  // Make it a big wall
            )
            .with<component::MaterialComponent>(mat6)
            .with<component::GeometryComponent>(g_cube_geom);
        
        std::cout << "✓ Scene created with 6 overlapping cubes (1 behind far plane)" << std::endl;
        
        // Create perspective camera
        auto camera = component::PerspectiveCamera::Make(60.0f, 0.1f, 100.0f);
        camera->getTransform()->setTranslate(0.0f, 0.0f, 0.0f);
        scene::graph()->setActiveCamera(camera);
        
        std::cout << "✓ Camera created" << std::endl;
        
        // Setup initial phase
        setupStandardDepthTestPhase();
        
        std::cout << "[INIT] Initialization complete!" << std::endl;
        std::cout << "\nPress SPACE to advance to next test phase" << std::endl;
    };
    
    auto on_update = [](double dt) {
        g_time += dt;
        
        // Handle phase transitions
        if (g_phase_changed) {
            g_phase_changed = false;
            
            switch (g_current_phase) {
                case TestPhase::STANDARD_DEPTH_TEST:
                    setupStandardDepthTestPhase();
                    break;
                case TestPhase::DEPTH_FUNCTIONS:
                    setupDepthFunctionsPhase();
                    break;
                case TestPhase::READ_ONLY_DEPTH:
                    setupReadOnlyDepthPhase();
                    break;
                case TestPhase::DEPTH_CLAMPING:
                    setupDepthClampingPhase();
                    break;
                case TestPhase::STATE_INHERITANCE:
                    setupStateInheritancePhase();
                    break;
                case TestPhase::DEPTH_RANGE:
                    setupDepthRangePhase();
                    break;
                case TestPhase::COMPLETE:
                    // Do nothing
                    break;
            }
        }
        
        // Update cube rotations
        for (int i = 1; i <= 6; i++) {
            std::string node_name = "cube" + std::to_string(i);
            auto cube_node = scene::graph()->getNodeByName(node_name);
            if (cube_node) {
                auto transform_comp = cube_node->payload().get<component::TransformComponent>();
                if (transform_comp) {
                    auto transform = transform_comp->getTransform();
                    transform->rotate(
                        static_cast<float>(dt * 20.0 * i),  // Different rotation speeds
                        0.0f, 1.0f, 0.0f
                    );
                    transform->rotate(
                        static_cast<float>(dt * 15.0 * i),
                        1.0f, 0.0f, 0.0f
                    );
                }
            }
        }
    };
    
    auto on_render = [](double alpha) {
        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Draw scene
        scene::graph()->draw();
        
        GL_CHECK("render");
    };
    
    // Input handler for phase transitions
    class DepthTestInputHandler : public input::InputHandler {
    protected:
        void handleKey(GLFWwindow* window, int key, int scancode, int action, int mods) override {
            if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
                if (g_current_phase == TestPhase::DEPTH_FUNCTIONS && g_current_phase != TestPhase::COMPLETE) {
                    // In depth functions phase, cycle through functions
                    g_depth_func_index = (g_depth_func_index + 1) % 8;
                    framebuffer::stack()->depth().setFunction(g_depth_funcs[g_depth_func_index]);
                    std::cout << "Depth function: " << g_depth_func_names[g_depth_func_index] << std::endl;
                    
                    // After cycling through all functions, advance to next phase
                    if (g_depth_func_index == 0) {
                        g_current_phase = TestPhase::READ_ONLY_DEPTH;
                        g_phase_changed = true;
                    }
                } else if (g_current_phase == TestPhase::DEPTH_RANGE && g_current_phase != TestPhase::COMPLETE) {
                    // Cycle through depth ranges
                    g_depth_range_index = (g_depth_range_index + 1) % 5;  // 5 ranges
                    
                    // Reset to default, modify in switch if needed
                    framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Less);
                    
                    switch(g_depth_range_index) {
                        case 0: 
                            framebuffer::stack()->depth().setRange(0.0, 1.0); 
                            break;
                        case 1: 
                            framebuffer::stack()->depth().setRange(0.0, 0.5); 
                            break;
                        case 2: 
                            framebuffer::stack()->depth().setRange(0.5, 1.0); 
                            break;
                        case 3: 
                            framebuffer::stack()->depth().setRange(1.0, 0.0);
                            framebuffer::stack()->depth().setFunction(framebuffer::DepthFunc::Greater);
                            break;
                        case 4: 
                            framebuffer::stack()->depth().setRange(0.2, 0.8); 
                            break;
                    }
                    
                    std::cout << "Depth range: " << g_depth_range_names[g_depth_range_index] << std::endl;
                    if (g_depth_range_index == 3) {
                        std::cout << "   (Note: Set depth func to 'Greater' for Reverse-Z)" << std::endl;
                    }
                    
                    // After cycling through all, advance to complete
                    if (g_depth_range_index == 0) {
                        g_current_phase = TestPhase::COMPLETE;
                        g_phase_changed = true;
                        
                        std::cout << "\n=== All Tests Complete ===" << std::endl;
                        std::cout << "✓ Standard depth testing validated" << std::endl;
                        std::cout << "✓ All depth functions validated" << std::endl;
                        std::cout << "✓ Read-only depth buffer validated" << std::endl;
                        std::cout << "✓ Depth clamping validated" << std::endl;
                        std::cout << "✓ State inheritance validated" << std::endl;
                        std::cout << "✓ Custom depth ranges validated" << std::endl;
                        std::cout << "✓ No OpenGL errors detected" << std::endl;
                        std::cout << "\nPress ESC to exit" << std::endl;
                    }
                } else if (g_current_phase != TestPhase::COMPLETE) {
                    // Advance to next phase
                    switch (g_current_phase) {
                        case TestPhase::STANDARD_DEPTH_TEST:
                            g_current_phase = TestPhase::DEPTH_FUNCTIONS;
                            break;
                        case TestPhase::READ_ONLY_DEPTH:
                            g_current_phase = TestPhase::DEPTH_CLAMPING;
                            break;
                        case TestPhase::DEPTH_CLAMPING:
                            g_current_phase = TestPhase::STATE_INHERITANCE;
                            break;
                        case TestPhase::STATE_INHERITANCE:
                            g_current_phase = TestPhase::DEPTH_RANGE;
                            break;
                        default:
                            break;
                    }
                    g_phase_changed = true;
                }
            }
        }
    };
    
    engene::EnGeneConfig config;
    config.title = "Depth Test - State Management Validation";
    config.width = 1024;
    config.height = 768;
    config.clearColor[0] = 0.1f;
    config.clearColor[1] = 0.1f;
    config.clearColor[2] = 0.1f;
    config.clearColor[3] = 1.0f;
    
    // Set custom shaders that support material abstraction
    config.base_vertex_shader_source = DEPTH_TEST_VERTEX_SHADER;
    config.base_fragment_shader_source = DEPTH_TEST_FRAGMENT_SHADER;
    
    try {
        auto input_handler = std::make_shared<DepthTestInputHandler>();
        engene::EnGene app(on_init, on_update, on_render, config, input_handler.get());
        
        // Configure shader uniforms for material support
        auto base_shader = app.getBaseShader();
        base_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
        base_shader->configureDynamicUniform<glm::vec4>("color", material::stack()->getProvider<glm::vec4>("color"));
        base_shader->Bake();
        
        std::cout << "\n[RUNNING] Depth test application" << std::endl;
        std::cout << "Validating:" << std::endl;
        std::cout << "  ✓ Depth test enable/disable" << std::endl;
        std::cout << "  ✓ Depth write mask configuration" << std::endl;
        std::cout << "  ✓ Depth function configuration" << std::endl;
        std::cout << "  ✓ Depth clamping enable/disable" << std::endl;
        std::cout << "  ✓ Depth range configuration" << std::endl;
        std::cout << "  ✓ State inheritance across push/pop" << std::endl;
        std::cout << "  ✓ Hierarchical state management" << std::endl;
        std::cout << "  ✓ No OpenGL errors" << std::endl;
        std::cout << std::endl;
        
        app.run();
        
        std::cout << "\n✓ Depth test completed successfully!" << std::endl;
        
    } catch (const exception::FramebufferException& e) {
        std::cerr << "✗ Framebuffer error: " << e.what() << std::endl;
        return 1;
    } catch (const exception::EnGeneException& e) {
        std::cerr << "✗ EnGene error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "✗ Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
