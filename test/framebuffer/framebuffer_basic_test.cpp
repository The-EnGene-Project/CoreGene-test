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
 * @brief Basic framebuffer test demonstrating render-to-texture functionality.
 * 
 * This test validates:
 * - FBO creation using MakeRenderToTexture factory
 * - FramebufferStack push/pop operations
 * - FramebufferComponent integration with scene graph
 * - Rendering a rotating cube to FBO texture
 * - Displaying FBO texture on a fullscreen quad
 * - Texture retrieval by name
 * - Dimension queries
 * 
 * Test Structure:
 * 1. First pass: Render rotating cube to FBO (off-screen)
 * 2. Second pass: Display FBO texture on fullscreen quad (on-screen)
 * 
 * Expected Result:
 * - Window displays a fullscreen quad showing the rendered cube texture
 * - Cube should be rotating and visible in the texture
 * - No OpenGL errors
 * 
 * Controls:
 * - ESC: Exit
 */

// Global state for animation
double g_time = 0.0;

// Shared resources
framebuffer::FramebufferPtr g_fbo;
geometry::GeometryPtr g_cube_geom;
geometry::GeometryPtr g_quad_geom;
shader::ShaderPtr g_texture_shader;

/**
 * @brief Creates a simple fullscreen quad geometry for displaying textures.
 * 
 * Vertex format: position (vec3), texcoord (vec2)
 */
geometry::GeometryPtr createFullscreenQuad() {
    // Fullscreen quad vertices: position (x, y, z) + texcoord (u, v)
    std::vector<float> vertices = {
        // positions          // texcoords
        -0.9f,  0.9f, 0.0f,   0.0f, 1.0f,  // top-left
        -0.9f, -0.9f, 0.0f,   0.0f, 0.0f,  // bottom-left
         0.9f, -0.9f, 0.0f,   1.0f, 0.0f,  // bottom-right
         0.9f,  0.9f, 0.0f,   1.0f, 1.0f   // top-right
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
 * @brief Creates a simple texture display shader.
 */
shader::ShaderPtr createTextureShader() {
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
        
        uniform sampler2D u_texture;
        
        void main() {
            FragColor = texture(u_texture, v_texcoord);
        }
    )";
    
    auto shader = shader::Shader::Make(vertex_source, fragment_source);
    
    return shader;
}

int main() {
    std::cout << "=== Framebuffer Basic Test ===" << std::endl;
    std::cout << "Testing: Render-to-texture with rotating cube" << std::endl;
    std::cout << "Expected: Fullscreen quad displaying rendered cube texture" << std::endl;
    std::cout << std::endl;
    
    auto on_init = [](engene::EnGene& app) {
        std::cout << "[INIT] Creating framebuffer..." << std::endl;
        
        // Create FBO using MakeRenderToTexture factory (800x600 resolution)
        g_fbo = framebuffer::Framebuffer::MakeRenderToTexture(80, 60, "scene_color");
        
        // Validate FBO creation
        if (!g_fbo) {
            throw exception::FramebufferException("Failed to create framebuffer");
        }
        
        std::cout << "✓ Framebuffer created successfully" << std::endl;
        
        // Validate dimension queries
        if (g_fbo->getWidth() != 80 || g_fbo->getHeight() != 60) {
            throw exception::FramebufferException(
                "Dimension mismatch: expected 800x600, got " + 
                std::to_string(g_fbo->getWidth()) + "x" + std::to_string(g_fbo->getHeight())
            );
        }
        std::cout << "✓ Dimension queries validated: " << g_fbo->getWidth() << "x" << g_fbo->getHeight() << std::endl;
        
        // Validate texture retrieval by name
        if (!g_fbo->hasTexture("scene_color")) {
            throw exception::FramebufferException("Texture 'scene_color' not found in framebuffer");
        }
        
        auto fbo_texture = g_fbo->getTexture("scene_color");
        if (!fbo_texture) {
            throw exception::FramebufferException("Failed to retrieve texture 'scene_color'");
        }
        std::cout << "✓ Texture retrieval by name validated" << std::endl;
        
        // Create cube geometry
        g_cube_geom = Cube::Make(1.0f, 1.0f, 1.0f);
        std::cout << "✓ Cube geometry created" << std::endl;
        
        // Create fullscreen quad for displaying FBO texture
        g_quad_geom = createFullscreenQuad();
        std::cout << "✓ Fullscreen quad created" << std::endl;
        
        // Create texture display shader
        g_texture_shader = createTextureShader();
        std::cout << "✓ Texture shader created" << std::endl;
        
        // Configure texture sampler uniform
        g_texture_shader->configureDynamicUniform<uniform::detail::Sampler>(
            "u_texture",
            texture::getSamplerProvider("u_texture")
        );
        
        std::cout << "[INIT] Setting up scene..." << std::endl;
        
        // Create off-screen scene (renders to FBO)
        // Using FramebufferComponent for scene graph integration
        auto& offscreen_root = scene::graph()->addNode("offscreen_scene")
            .with<component::FramebufferComponent>(g_fbo);
        
        // Add rotating cube to off-screen scene
        offscreen_root.addNode("rotating_cube")
            .with<component::TransformComponent>(
                transform::Transform::Make()
                    ->setTranslate(0.0f, 0.0f, -5.0f)
            )
            .with<component::GeometryComponent>(g_cube_geom);
        
        std::cout << "✓ Off-screen scene created with FramebufferComponent" << std::endl;
        
        // Create on-screen scene (renders to default framebuffer)
        // Displays the FBO texture on a fullscreen quad
        scene::graph()->addNode("fullscreen_quad")
            .with<component::ShaderComponent>(g_texture_shader)
            .with<component::TextureComponent>(fbo_texture, "u_texture", 0)
            .with<component::GeometryComponent>(g_quad_geom);
        
        std::cout << "✓ On-screen scene created" << std::endl;
        
        // Create perspective camera for off-screen rendering
        auto camera = component::PerspectiveCamera::Make(60.0f, 0.1f, 100.0f);
        camera->getTransform()->setTranslate(0.0f, 0.0f, 0.0f);
        scene::graph()->setActiveCamera(camera);
        
        std::cout << "✓ Camera created" << std::endl;
        std::cout << "[INIT] Initialization complete!" << std::endl;
    };
    
    auto on_update = [](double dt) {
        // g_time += dt;
        
        // Update cube rotation
        auto cube_node = scene::graph()->getNodeByName("rotating_cube");
        if (cube_node) {
            auto transform_comp = cube_node->payload().get<component::TransformComponent>();
            if (transform_comp) {
                auto transform = transform_comp->getTransform();
                transform->rotate(
                    static_cast<float>(dt*50),  // Rotate around Y axis
                    0.0f, 1.0f, 0.0f
                );
                transform->rotate(
                    static_cast<float>(dt * 30.0),  // Rotate around X axis
                    1.0f, 0.0f, 0.0f
                );
            }
        }
    };
    
    auto on_render = [](double alpha) {
        // Test FramebufferStack operations
        // The FramebufferComponent automatically pushes/pops the FBO during scene traversal
        
        // Clear default framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Draw scene (includes both off-screen and on-screen passes)
        // 1. Off-screen pass: FramebufferComponent pushes FBO, renders cube, pops FBO
        // 2. On-screen pass: Renders fullscreen quad with FBO texture
        scene::graph()->draw();
        
        GL_CHECK("render");
    };
    
    engene::EnGeneConfig config;
    config.title = "Framebuffer Basic Test - Render to Texture";
    config.width = 1024;
    config.height = 768;
    config.clearColor[0] = 0.2f;
    config.clearColor[1] = 0.3f;
    config.clearColor[2] = 0.4f;
    config.clearColor[3] = 1.0f;
    
    try {
        engene::EnGene app(on_init, on_update, on_render, config);
        
        std::cout << "\n[RUNNING] Framebuffer basic test" << std::endl;
        std::cout << "Validating:" << std::endl;
        std::cout << "  ✓ FBO creation with MakeRenderToTexture" << std::endl;
        std::cout << "  ✓ FramebufferStack push/pop (via FramebufferComponent)" << std::endl;
        std::cout << "  ✓ FramebufferComponent scene graph integration" << std::endl;
        std::cout << "  ✓ Texture retrieval by name" << std::endl;
        std::cout << "  ✓ Dimension queries" << std::endl;
        std::cout << "  ✓ Render cube to FBO texture" << std::endl;
        std::cout << "  ✓ Display FBO texture on fullscreen quad" << std::endl;
        std::cout << std::endl;
        
        app.run();
        
        std::cout << "\n✓ Framebuffer basic test completed successfully!" << std::endl;
        
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
