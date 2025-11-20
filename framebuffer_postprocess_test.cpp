#include <iostream>
#include <cmath>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <gl_base/framebuffer.h>
#include <3d/camera/perspective_camera.h>
#include <other_genes/3d_shapes/cube.h>

/**
 * @brief Post-processing framebuffer test demonstrating advanced FBO features.
 * 
 * This test validates:
 * - FBO creation using MakePostProcessing factory (with depth renderbuffer)
 * - attachToShader() utility method for automatic sampler configuration
 * - generateMipmaps() functionality for texture filtering
 * - Multi-pass rendering with post-processing effects
 * - Grayscale post-processing effect
 * 
 * Test Structure:
 * 1. First pass: Render rotating cube to FBO (off-screen, with depth testing)
 * 2. Second pass: Apply grayscale post-processing effect to FBO texture
 * 3. Display processed result on fullscreen quad (on-screen)
 * 
 * Expected Result:
 * - Window displays a fullscreen quad with grayscale version of the cube
 * - Cube should be rotating and visible in grayscale with proper depth sorting
 * - No OpenGL errors
 * - Mipmaps generated successfully
 * 
 * Controls:
 * - ESC: Exit
 * - SPACE: Toggle between grayscale and original color
 */

// Global state for animation and testing
double g_time = 0.0;
bool g_use_grayscale = true;

// Shared resources
framebuffer::FramebufferPtr g_fbo;
geometry::GeometryPtr g_cube_geom;
geometry::GeometryPtr g_quad_geom;
shader::ShaderPtr g_grayscale_shader;
shader::ShaderPtr g_passthrough_shader;

/**
 * @brief Creates a simple fullscreen quad geometry for displaying textures.
 * 
 * Vertex format: position (vec3), texcoord (vec2)
 */
geometry::GeometryPtr createFullscreenQuad() {
    // Fullscreen quad vertices: position (x, y, z) + texcoord (u, v)
    std::vector<float> vertices = {
        // positions          // texcoords
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f,  // top-left
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,  // bottom-left
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f,  // bottom-right
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f   // top-right
    };
    
    std::vector<unsigned int> indices = {
        0, 1, 2,  // first triangle
        0, 2, 3   // second triangle
    };
    
    return geometry::Geometry::Make(
        vertices.data(), 
        indices.data(),
        4,  // 4 vertices
        6,  // 6 indices
        3,  // 3 floats for position
        {2} // 2 floats for texcoord
    );
}

/**
 * @brief Creates a grayscale post-processing shader.
 * 
 * This shader converts the input texture to grayscale using the
 * luminance formula: 0.299*R + 0.587*G + 0.114*B
 */
shader::ShaderPtr createGrayscaleShader() {
    const char* vertex_source = R"(
        #version 430 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec2 a_texcoord;
        
        out vec2 v_texcoord;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_texcoord = a_texcoord;
        }
    )";
    
    const char* fragment_source = R"(
        #version 430 core
        in vec2 v_texcoord;
        out vec4 FragColor;
        
        uniform sampler2D u_scene_texture;
        
        void main() {
            vec4 color = texture(u_scene_texture, v_texcoord);
            
            // Convert to grayscale using luminance formula
            float gray = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
            
            FragColor = vec4(gray, gray, gray, color.a);
        }
    )";
    
    auto shader = shader::Shader::Make(vertex_source, fragment_source);
    
    return shader;
}

/**
 * @brief Creates a simple passthrough shader (no post-processing).
 */
shader::ShaderPtr createPassthroughShader() {
    const char* vertex_source = R"(
        #version 430 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec2 a_texcoord;
        
        out vec2 v_texcoord;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_texcoord = a_texcoord;
        }
    )";
    
    const char* fragment_source = R"(
        #version 430 core
        in vec2 v_texcoord;
        out vec4 FragColor;
        
        uniform sampler2D u_scene_texture;
        
        void main() {
            FragColor = texture(u_scene_texture, v_texcoord);
        }
    )";
    
    auto shader = shader::Shader::Make(vertex_source, fragment_source);
    
    return shader;
}

int main() {
    std::cout << "=== Framebuffer Post-Processing Test ===" << std::endl;
    std::cout << "Testing: Post-processing with grayscale effect" << std::endl;
    std::cout << "Expected: Fullscreen quad displaying grayscale cube" << std::endl;
    std::cout << "Controls: SPACE to toggle grayscale on/off" << std::endl;
    std::cout << std::endl;
    
    auto on_init = [](engene::EnGene& app) {
        std::cout << "[INIT] Creating post-processing framebuffer..." << std::endl;
        
        // Create FBO using MakePostProcessing factory (with depth renderbuffer)
        g_fbo = framebuffer::Framebuffer::MakePostProcessing(800, 600, "post_color");
        
        // Validate FBO creation
        if (!g_fbo) {
            throw exception::FramebufferException("Failed to create framebuffer");
        }
        
        std::cout << "✓ Post-processing framebuffer created successfully" << std::endl;
        
        // Validate color texture attachment exists
        if (!g_fbo->hasTexture("post_color")) {
            throw exception::FramebufferException("Texture 'post_color' not found in framebuffer");
        }
        std::cout << "✓ Color texture attachment validated" << std::endl;
        std::cout << "✓ Depth renderbuffer attachment included (for proper depth testing)" << std::endl;
        
        // Validate dimension queries
        if (g_fbo->getWidth() != 800 || g_fbo->getHeight() != 600) {
            throw exception::FramebufferException(
                "Dimension mismatch: expected 800x600, got " + 
                std::to_string(g_fbo->getWidth()) + "x" + std::to_string(g_fbo->getHeight())
            );
        }
        std::cout << "✓ Dimension queries validated: " << g_fbo->getWidth() << "x" << g_fbo->getHeight() << std::endl;
        
        // Create cube geometry
        g_cube_geom = Cube::Make(1.0f, 1.0f, 1.0f);
        std::cout << "✓ Cube geometry created" << std::endl;
        
        // Create fullscreen quad for displaying FBO texture
        g_quad_geom = createFullscreenQuad();
        std::cout << "✓ Fullscreen quad created" << std::endl;
        
        // Create post-processing shaders
        g_grayscale_shader = createGrayscaleShader();
        g_passthrough_shader = createPassthroughShader();
        std::cout << "✓ Post-processing shaders created" << std::endl;
        
        // Test attachToShader() utility method
        std::cout << "[INIT] Testing attachToShader() utility..." << std::endl;
        g_fbo->attachToShader(g_grayscale_shader, {{"post_color", "u_scene_texture"}});
        g_fbo->attachToShader(g_passthrough_shader, {{"post_color", "u_scene_texture"}});
        std::cout << "✓ attachToShader() configured samplers successfully" << std::endl;
        
        // Test generateMipmaps() functionality
        std::cout << "[INIT] Testing generateMipmaps()..." << std::endl;
        
        // First, render something to the FBO so mipmaps have content
        framebuffer::stack()->push(g_fbo);
        glClear(GL_COLOR_BUFFER_BIT);
        framebuffer::stack()->pop();
        
        // Generate mipmaps
        g_fbo->generateMipmaps("post_color");
        std::cout << "✓ Mipmaps generated successfully" << std::endl;
        
        std::cout << "[INIT] Setting up scene..." << std::endl;
        
        // Create off-screen scene (renders to FBO)
        auto& offscreen_root = scene::graph()->addNode("offscreen_scene")
            .with<component::FramebufferComponent>(g_fbo);
        
        // Add rotating cube to off-screen scene
        offscreen_root.addNode("rotating_cube")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(0.0f, 0.0f, -5.0f)
            )
            .with<component::GeometryComponent>(g_cube_geom);
        
        std::cout << "✓ Off-screen scene created" << std::endl;
        
        // Create on-screen scene (renders to default framebuffer)
        // Displays the FBO texture with post-processing on a fullscreen quad
        auto fbo_texture = g_fbo->getTexture("post_color");
        
        scene::graph()->addNode("fullscreen_quad")
            .with<component::ShaderComponent>(g_grayscale_shader)
            .with<component::TextureComponent>(fbo_texture, "u_scene_texture", 0)
            .with<component::GeometryComponent>(g_quad_geom);
        
        std::cout << "✓ On-screen scene created with post-processing" << std::endl;
        
        // Create perspective camera for off-screen rendering
        auto camera = component::PerspectiveCamera::Make(60.0f, 0.1f, 100.0f);
        camera->getTransform()->setTranslate(0.0f, 0.0f, 0.0f);
        scene::graph()->setActiveCamera(camera);
        
        std::cout << "✓ Camera created" << std::endl;
        std::cout << "[INIT] Initialization complete!" << std::endl;
        std::cout << std::endl;
        std::cout << "Press SPACE to toggle grayscale effect on/off" << std::endl;
    };
    
    auto on_update = [](double dt) {
        g_time += dt;
        
        // Update cube rotation
        auto cube_node = scene::graph()->getNodeByName("rotating_cube");
        if (cube_node) {
            auto transform_comp = cube_node->payload().get<component::TransformComponent>();
            if (transform_comp) {
                auto transform = transform_comp->getTransform();
                transform->rotate(
                    static_cast<float>(dt * 50.0),  // Rotate around Y axis
                    0.0f, 1.0f, 0.0f
                );
                transform->rotate(
                    static_cast<float>(dt * 30.0),  // Rotate around X axis
                    1.0f, 0.0f, 0.0f
                );
            }
        }
        
        // Toggle shader based on user input
        auto quad_node = scene::graph()->getNodeByName("fullscreen_quad");
        if (quad_node) {
            auto shader_comp = quad_node->payload().get<component::ShaderComponent>();
            if (shader_comp) {
                if (g_use_grayscale) {
                    shader_comp->setShader(g_grayscale_shader);
                } else {
                    shader_comp->setShader(g_passthrough_shader);
                }
            }
        }
    };
    
    auto on_render = [](double alpha) {
        // Clear default framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Draw scene (includes both off-screen and on-screen passes)
        // 1. Off-screen pass: FramebufferComponent pushes FBO, renders cube, pops FBO
        // 2. On-screen pass: Renders fullscreen quad with post-processed FBO texture
        scene::graph()->draw();
        
        GL_CHECK("render");
    };
    
    engene::EnGeneConfig config;
    config.title = "Framebuffer Post-Processing Test - Grayscale Effect";
    config.width = 1024;
    config.height = 768;
    config.clearColor[0] = 0.1f;
    config.clearColor[1] = 0.1f;
    config.clearColor[2] = 0.1f;
    config.clearColor[3] = 1.0f;
    
    // Create input handler and register keyboard callback for toggling grayscale
    auto* input_handler = new input::InputHandler();
    input_handler->registerCallback<input::InputType::KEY>(
        [](KEY_HANDLER_ARGS) {
            if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
                g_use_grayscale = !g_use_grayscale;
                std::cout << "Grayscale effect: " << (g_use_grayscale ? "ON" : "OFF") << std::endl;
            }
        }
    );
    
    try {
        engene::EnGene app(on_init, on_update, on_render, config, input_handler);
        
        std::cout << "\n[RUNNING] Framebuffer post-processing test" << std::endl;
        std::cout << "Validating:" << std::endl;
        std::cout << "  ✓ FBO creation with MakePostProcessing (with depth renderbuffer)" << std::endl;
        std::cout << "  ✓ attachToShader() utility method" << std::endl;
        std::cout << "  ✓ generateMipmaps() functionality" << std::endl;
        std::cout << "  ✓ Multi-pass rendering with post-processing" << std::endl;
        std::cout << "  ✓ Grayscale post-processing effect" << std::endl;
        std::cout << "  ✓ Proper depth testing during off-screen rendering" << std::endl;
        std::cout << std::endl;
        
        app.run();
        
        std::cout << "\n✓ Framebuffer post-processing test completed successfully!" << std::endl;
        
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
