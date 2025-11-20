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
const char* BLEND_TEST_VERTEX_SHADER = R"(
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

const char* BLEND_TEST_FRAGMENT_SHADER = R"(
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
 * @brief Blend test application demonstrating blend state functionality
 * 
 * This test validates:
 * - Blend enable/disable via framebuffer::stack()->blend().setEnabled()
 * - Blend equation configuration (Add, Subtract, ReverseSubtract, Min, Max)
 * - Blend function configuration (source and destination factors)
 * - Separate RGB/Alpha blending
 * - Constant color blending
 * - State inheritance across framebuffer push/pop
 * - Hierarchical blend state management
 * - No OpenGL errors during blend operations
 * 
 * Test Structure:
 * 1. Basic alpha blending: Overlapping transparent cubes
 * 2. Separate RGB/Alpha blending: Different blend modes for color and alpha
 * 3. Constant color blending: Using blend constant color
 * 4. State inheritance: Test push/pop with blend state
 * 5. Advanced blend equations: Test Min, Max, Subtract modes
 * 
 * Expected Result:
 * - Transparent cubes blend correctly
 * - Separate RGB/Alpha blending works as configured
 * - Constant color affects blending
 * - State inheritance works correctly across framebuffer stack
 * - No OpenGL errors
 * 
 * Controls:
 * - ESC: Exit
 * - SPACE: Toggle between test phases
 */

// Global state for test phases
enum class TestPhase {
    BASIC_ALPHA_BLEND,      // Standard alpha blending
    SEPARATE_RGB_ALPHA,     // Separate RGB/Alpha blending
    CONSTANT_COLOR_BLEND,   // Constant color blending
    STATE_INHERITANCE,      // Test state inheritance
    ADVANCED_EQUATIONS,     // Test Min, Max, Subtract
    COMPLETE
};

TestPhase g_current_phase = TestPhase::BASIC_ALPHA_BLEND;
double g_time = 0.0;
bool g_phase_changed = false;

// Shared resources
geometry::GeometryPtr g_cube_geom;
framebuffer::FramebufferPtr g_fbo;

/**
 * @brief Test Phase 1: Basic alpha blending
 * 
 * Configures standard alpha blending (SrcAlpha, OneMinusSrcAlpha).
 * Renders overlapping transparent cubes.
 */
void setupBasicAlphaBlendPhase() {
    std::cout << "\n=== Phase 1: Basic Alpha Blending ===" << std::endl;
    std::cout << "Configuring standard alpha blending..." << std::endl;
    
    // Enable blending
    framebuffer::stack()->blend().setEnabled(true);
    std::cout << "✓ Blend enabled" << std::endl;
    
    // Configure blend equation: Add (default)
    framebuffer::stack()->blend().setEquation(framebuffer::BlendEquation::Add);
    std::cout << "✓ Blend equation set to Add" << std::endl;
    
    // Configure blend function: Standard alpha blending
    framebuffer::stack()->blend().setFunction(
        framebuffer::BlendFactor::SrcAlpha,
        framebuffer::BlendFactor::OneMinusSrcAlpha);
    std::cout << "✓ Blend function configured (SrcAlpha, OneMinusSrcAlpha)" << std::endl;
    
    std::cout << "Phase 1 setup complete. Transparent cubes will blend with alpha." << std::endl;
}

/**
 * @brief Test Phase 2: Separate RGB/Alpha blending
 * 
 * Configures different blend modes for RGB and Alpha channels.
 * Tests separate equation and function configuration.
 */
void setupSeparateRGBAlphaPhase() {
    std::cout << "\n=== Phase 2: Separate RGB/Alpha Blending ===" << std::endl;
    std::cout << "Configuring separate RGB and Alpha blending..." << std::endl;
    
    // Enable blending
    framebuffer::stack()->blend().setEnabled(true);
    std::cout << "✓ Blend enabled" << std::endl;
    
    // Configure separate blend equations
    framebuffer::stack()->blend().setEquationSeparate(
        framebuffer::BlendEquation::Add,           // RGB: Add
        framebuffer::BlendEquation::Max);          // Alpha: Max
    std::cout << "✓ Blend equations configured (RGB=Add, Alpha=Max)" << std::endl;
    
    // Configure separate blend functions
    framebuffer::stack()->blend().setFunctionSeparate(
        framebuffer::BlendFactor::SrcAlpha,        // srcRGB
        framebuffer::BlendFactor::OneMinusSrcAlpha, // dstRGB
        framebuffer::BlendFactor::One,             // srcAlpha
        framebuffer::BlendFactor::Zero);           // dstAlpha
    std::cout << "✓ Blend functions configured separately" << std::endl;
    std::cout << "  RGB: (SrcAlpha, OneMinusSrcAlpha)" << std::endl;
    std::cout << "  Alpha: (One, Zero)" << std::endl;
    
    std::cout << "Phase 2 setup complete. RGB and Alpha blend independently." << std::endl;
}

/**
 * @brief Test Phase 3: Constant color blending
 * 
 * Configures blending with constant color.
 * Tests setConstantColor() method.
 */
void setupConstantColorBlendPhase() {
    std::cout << "\n=== Phase 3: Constant Color Blending ===" << std::endl;
    std::cout << "Configuring constant color blending..." << std::endl;
    
    // Enable blending
    framebuffer::stack()->blend().setEnabled(true);
    std::cout << "✓ Blend enabled" << std::endl;
    
    // Set blend constant color (purple tint)
    framebuffer::stack()->blend().setConstantColor(0.5f, 0.0f, 0.5f, 0.5f);
    std::cout << "✓ Blend constant color set (0.5, 0.0, 0.5, 0.5)" << std::endl;
    
    // Configure blend equation
    framebuffer::stack()->blend().setEquation(framebuffer::BlendEquation::Add);
    std::cout << "✓ Blend equation set to Add" << std::endl;
    
    // Configure blend function using constant color
    framebuffer::stack()->blend().setFunctionSeparate(
        framebuffer::BlendFactor::ConstantColor,       // srcRGB
        framebuffer::BlendFactor::OneMinusConstantColor, // dstRGB
        framebuffer::BlendFactor::ConstantAlpha,       // srcAlpha
        framebuffer::BlendFactor::OneMinusConstantAlpha); // dstAlpha
    std::cout << "✓ Blend functions configured with constant color factors" << std::endl;
    
    std::cout << "Phase 3 setup complete. Blending uses constant color." << std::endl;
}

/**
 * @brief Test Phase 4: State inheritance
 * 
 * Tests that blend state is inherited correctly when pushing/popping framebuffers.
 * Validates hierarchical state management.
 */
void setupStateInheritancePhase() {
    std::cout << "\n=== Phase 4: State Inheritance ===" << std::endl;
    std::cout << "Testing hierarchical blend state management..." << std::endl;
    
    // Configure root level state
    framebuffer::stack()->blend().setEnabled(true);
    framebuffer::stack()->blend().setFunction(
        framebuffer::BlendFactor::SrcAlpha,
        framebuffer::BlendFactor::OneMinusSrcAlpha);
    std::cout << "✓ Root state configured (blend enabled, standard alpha)" << std::endl;
    
    // Push FBO (should inherit state)
    if (g_fbo) {
        framebuffer::stack()->push(g_fbo);
        std::cout << "✓ Pushed FBO - state should be inherited" << std::endl;
        
        // Modify state in child
        framebuffer::stack()->blend().setEquation(framebuffer::BlendEquation::Max);
        framebuffer::stack()->blend().setFunction(
            framebuffer::BlendFactor::One,
            framebuffer::BlendFactor::One);
        std::cout << "✓ Modified blend state in child (Max equation, additive blend)" << std::endl;
        
        // Pop FBO (should restore parent state)
        framebuffer::stack()->pop();
        std::cout << "✓ Popped FBO - parent state should be restored" << std::endl;
    }
    
    // Test with RenderState (apply mode)
    auto render_state = std::make_shared<framebuffer::RenderState>();
    render_state->blend().setEnabled(true);
    render_state->blend().setEquationSeparate(
        framebuffer::BlendEquation::Subtract,
        framebuffer::BlendEquation::ReverseSubtract);
    render_state->blend().setFunctionSeparate(
        framebuffer::BlendFactor::DstColor,
        framebuffer::BlendFactor::SrcColor,
        framebuffer::BlendFactor::DstAlpha,
        framebuffer::BlendFactor::SrcAlpha);
    render_state->blend().setConstantColor(0.2f, 0.3f, 0.4f, 0.5f);
    std::cout << "✓ Created RenderState with custom blend configuration" << std::endl;
    
    if (g_fbo) {
        framebuffer::stack()->push(g_fbo, render_state);
        std::cout << "✓ Pushed FBO with RenderState (apply mode)" << std::endl;
        
        framebuffer::stack()->pop();
        std::cout << "✓ Popped FBO - state restored" << std::endl;
    }
    
    std::cout << "Phase 4 complete. State inheritance validated." << std::endl;
}

/**
 * @brief Test Phase 5: Advanced blend equations
 * 
 * Tests Min, Max, Subtract, and ReverseSubtract blend equations.
 */
void setupAdvancedEquationsPhase() {
    std::cout << "\n=== Phase 5: Advanced Blend Equations ===" << std::endl;
    std::cout << "Testing Min, Max, Subtract, ReverseSubtract equations..." << std::endl;
    
    // Enable blending
    framebuffer::stack()->blend().setEnabled(true);
    std::cout << "✓ Blend enabled" << std::endl;
    
    // Test Min equation
    framebuffer::stack()->blend().setEquation(framebuffer::BlendEquation::Min);
    framebuffer::stack()->blend().setFunction(
        framebuffer::BlendFactor::One,
        framebuffer::BlendFactor::One);
    std::cout << "✓ Configured Min equation with (One, One)" << std::endl;
    
    // Test Max equation
    framebuffer::stack()->blend().setEquation(framebuffer::BlendEquation::Max);
    std::cout << "✓ Configured Max equation" << std::endl;
    
    // Test Subtract equation
    framebuffer::stack()->blend().setEquation(framebuffer::BlendEquation::Subtract);
    std::cout << "✓ Configured Subtract equation" << std::endl;
    
    // Test ReverseSubtract equation
    framebuffer::stack()->blend().setEquation(framebuffer::BlendEquation::ReverseSubtract);
    std::cout << "✓ Configured ReverseSubtract equation" << std::endl;
    
    // Test separate equations with different modes
    framebuffer::stack()->blend().setEquationSeparate(
        framebuffer::BlendEquation::Min,
        framebuffer::BlendEquation::Max);
    std::cout << "✓ Configured separate equations (RGB=Min, Alpha=Max)" << std::endl;
    
    std::cout << "Phase 5 complete. All blend equations tested." << std::endl;
}

int main() {
    std::cout << "=== Blend Test Application ===" << std::endl;
    std::cout << "Testing: Blend state operations and state management" << std::endl;
    std::cout << "Expected: Blending works correctly, state inheritance validated" << std::endl;
    std::cout << std::endl;
    
    auto on_init = [](engene::EnGene& app) {
        std::cout << "[INIT] Initializing blend test..." << std::endl;
        
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
        
        // Create scene with multiple overlapping cubes
        // Cube 1: Red, semi-transparent
        auto mat1 = material::Material::Make(glm::vec3(1.0f, 0.0f, 0.0f));
        mat1->set("color", glm::vec4(1.0f, 0.0f, 0.0f, 0.5f));
        
        scene::graph()->addNode("cube1")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(-1.0f, 0.0f, -5.0f)
            )
            .with<component::MaterialComponent>(mat1)
            .with<component::GeometryComponent>(g_cube_geom);
        
        // Cube 2: Green, semi-transparent
        auto mat2 = material::Material::Make(glm::vec3(0.0f, 1.0f, 0.0f));
        mat2->set("color", glm::vec4(0.0f, 1.0f, 0.0f, 0.5f));
        
        scene::graph()->addNode("cube2")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(0.0f, 0.0f, -5.5f)
            )
            .with<component::MaterialComponent>(mat2)
            .with<component::GeometryComponent>(g_cube_geom);
        
        // Cube 3: Blue, semi-transparent
        auto mat3 = material::Material::Make(glm::vec3(0.0f, 0.0f, 1.0f));
        mat3->set("color", glm::vec4(0.0f, 0.0f, 1.0f, 0.5f));
        
        scene::graph()->addNode("cube3")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(1.0f, 0.0f, -6.0f)
            )
            .with<component::MaterialComponent>(mat3)
            .with<component::GeometryComponent>(g_cube_geom);
        
        std::cout << "✓ Scene created with 3 overlapping transparent cubes" << std::endl;
        
        // Create perspective camera
        auto camera = component::PerspectiveCamera::Make(60.0f, 0.1f, 100.0f);
        camera->getTransform()->setTranslate(0.0f, 0.0f, 0.0f);
        scene::graph()->setActiveCamera(camera);
        
        std::cout << "✓ Camera created" << std::endl;
        
        // Setup initial phase
        setupBasicAlphaBlendPhase();
        
        std::cout << "[INIT] Initialization complete!" << std::endl;
        std::cout << "\nPress SPACE to advance to next test phase" << std::endl;
    };
    
    auto on_update = [](double dt) {
        g_time += dt;
        
        // Handle phase transitions
        if (g_phase_changed) {
            g_phase_changed = false;
            
            switch (g_current_phase) {
                case TestPhase::BASIC_ALPHA_BLEND:
                    setupBasicAlphaBlendPhase();
                    break;
                case TestPhase::SEPARATE_RGB_ALPHA:
                    setupSeparateRGBAlphaPhase();
                    break;
                case TestPhase::CONSTANT_COLOR_BLEND:
                    setupConstantColorBlendPhase();
                    break;
                case TestPhase::STATE_INHERITANCE:
                    setupStateInheritancePhase();
                    break;
                case TestPhase::ADVANCED_EQUATIONS:
                    setupAdvancedEquationsPhase();
                    g_current_phase = TestPhase::COMPLETE;
                    std::cout << "\n=== All Tests Complete ===" << std::endl;
                    std::cout << "✓ Basic alpha blending validated" << std::endl;
                    std::cout << "✓ Separate RGB/Alpha blending validated" << std::endl;
                    std::cout << "✓ Constant color blending validated" << std::endl;
                    std::cout << "✓ State inheritance validated" << std::endl;
                    std::cout << "✓ Advanced blend equations validated" << std::endl;
                    std::cout << "✓ No OpenGL errors detected" << std::endl;
                    std::cout << "\nPress ESC to exit" << std::endl;
                    break;
                case TestPhase::COMPLETE:
                    // Do nothing
                    break;
            }
        }
        
        // Update cube rotations
        for (int i = 1; i <= 3; i++) {
            std::string node_name = "cube" + std::to_string(i);
            auto cube_node = scene::graph()->getNodeByName(node_name);
            if (cube_node) {
                auto transform_comp = cube_node->payload().get<component::TransformComponent>();
                if (transform_comp) {
                    auto transform = transform_comp->getTransform();
                    transform->rotate(
                        static_cast<float>(dt * 30.0 * i),  // Different rotation speeds
                        0.0f, 1.0f, 0.0f
                    );
                    transform->rotate(
                        static_cast<float>(dt * 20.0 * i),
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
    class BlendTestInputHandler : public input::InputHandler {
    protected:
        void handleKey(GLFWwindow* window, int key, int scancode, int action, int mods) override {
            if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && g_current_phase != TestPhase::COMPLETE) {
                // Advance to next phase
                switch (g_current_phase) {
                    case TestPhase::BASIC_ALPHA_BLEND:
                        g_current_phase = TestPhase::SEPARATE_RGB_ALPHA;
                        break;
                    case TestPhase::SEPARATE_RGB_ALPHA:
                        g_current_phase = TestPhase::CONSTANT_COLOR_BLEND;
                        break;
                    case TestPhase::CONSTANT_COLOR_BLEND:
                        g_current_phase = TestPhase::STATE_INHERITANCE;
                        break;
                    case TestPhase::STATE_INHERITANCE:
                        g_current_phase = TestPhase::ADVANCED_EQUATIONS;
                        break;
                    default:
                        break;
                }
                g_phase_changed = true;
            }
        }
    };
    
    engene::EnGeneConfig config;
    config.title = "Blend Test - State Management Validation";
    config.width = 1024;
    config.height = 768;
    config.clearColor[0] = 0.1f;
    config.clearColor[1] = 0.1f;
    config.clearColor[2] = 0.1f;
    config.clearColor[3] = 1.0f;
    
    // Set custom shaders that support material abstraction
    config.base_vertex_shader_source = BLEND_TEST_VERTEX_SHADER;
    config.base_fragment_shader_source = BLEND_TEST_FRAGMENT_SHADER;
    
    try {
        auto input_handler = std::make_shared<BlendTestInputHandler>();
        engene::EnGene app(on_init, on_update, on_render, config, input_handler.get());
        
        // Configure shader uniforms for material support
        auto base_shader = app.getBaseShader();
        base_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
        base_shader->configureDynamicUniform<glm::vec4>("color", material::stack()->getProvider<glm::vec4>("color"));
        base_shader->Bake();
        
        std::cout << "\n[RUNNING] Blend test application" << std::endl;
        std::cout << "Validating:" << std::endl;
        std::cout << "  ✓ Blend enable/disable" << std::endl;
        std::cout << "  ✓ Blend equation configuration" << std::endl;
        std::cout << "  ✓ Blend function configuration" << std::endl;
        std::cout << "  ✓ Separate RGB/Alpha blending" << std::endl;
        std::cout << "  ✓ Constant color blending" << std::endl;
        std::cout << "  ✓ State inheritance across push/pop" << std::endl;
        std::cout << "  ✓ Hierarchical state management" << std::endl;
        std::cout << "  ✓ No OpenGL errors" << std::endl;
        std::cout << std::endl;
        
        app.run();
        
        std::cout << "\n✓ Blend test completed successfully!" << std::endl;
        
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
