#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <iostream>

int main() {
    std::cout << "=== EnGene Environment Configuration Test ===" << std::endl;
    std::cout << std::endl;

    // Define custom shaders as raw strings
    const char* vertex_shader = R"(
        #version 410 core
        layout (location = 0) in vec4 vertex;
        layout (location = 1) in vec4 icolor;

        out vec4 vertexColor;

        // Camera UBO (required by EnGene)
        layout (std140) uniform CameraMatrices {
            mat4 view;
            mat4 projection;
        };

        // Model matrix (required by default shader)
        uniform mat4 u_model;

        void main() {
            vertexColor = icolor;
            gl_Position = projection * view * u_model * vertex;
        }
    )";

    const char* fragment_shader = R"(
        #version 410 core
        
        in vec4 vertexColor;
        out vec4 fragColor;

        void main() {
            fragColor = vertexColor;
        }
    )";

    try {
        // Define initialization callback
        auto on_initialize = [](engene::EnGene& app) {
            std::cout << "[INIT] EnGene initialized successfully!" << std::endl;
            std::cout << "[INIT] OpenGL context created" << std::endl;
            
            // Create a simple triangle geometry with vec4 positions (x, y, z, w)
            std::vector<float> vertices = {
                // positions (x, y, z, w)     // colors (r, g, b, a)
                0.0f,  0.5f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.0f,  // top (red)
               -0.5f, -0.5f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.0f,  // bottom-left (green)
                0.5f, -0.5f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.0f   // bottom-right (blue)
            };
            std::vector<unsigned int> indices = {0, 1, 2};
            
            std::cout << "[INIT] Creating triangle geometry..." << std::endl;
            auto triangle = geometry::Geometry::Make(
                vertices.data(), indices.data(), 
                3, 3,  // 3 vertices, 3 indices
                4,     // 4 floats for position (x, y, z, w)
                {4}    // 4 floats for color (r, g, b, a)
            );
            
            std::cout << "[INIT] Building scene graph..." << std::endl;
            scene::graph()->addNode("Triangle")
                .with<component::GeometryComponent>(triangle);
            
            std::cout << "[INIT] Scene setup complete!" << std::endl;
            std::cout << std::endl;
            std::cout << "Press ESC or close window to exit..." << std::endl;
        };
        
        // Define fixed update callback
        auto on_fixed_update = [](double dt) {
            // No physics needed for this test
        };
        
        // Define render callback
        auto on_render = [](double alpha) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene::graph()->draw();
            GL_CHECK("render");
        };
        
        // Configure the engine with custom shaders
        engene::EnGeneConfig config;
        config.title = "EnGene Environment Test";
        config.width = 800;
        config.height = 600;
        config.clearColor[0] = 0.1f;
        config.clearColor[1] = 0.1f;
        config.clearColor[2] = 0.15f;
        config.clearColor[3] = 1.0f;
        
        // Set custom shaders (matching default shader format)
        config.base_vertex_shader_source = vertex_shader;
        config.base_fragment_shader_source = fragment_shader;
        
        std::cout << "[TEST] Creating EnGene application..." << std::endl;
        engene::EnGene app(on_initialize, on_fixed_update, on_render, config);
        
        // Configure u_model uniform for custom shader (must be done after construction)
        std::cout << "[TEST] Configuring shader uniforms..." << std::endl;
        auto base_shader = app.getBaseShader();
        base_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
        base_shader->Bake();  // Re-bake to apply uniform configuration
        
        std::cout << "[TEST] Starting main loop..." << std::endl;
        app.run();
        
        std::cout << std::endl;
        std::cout << "[TEST] Application closed successfully!" << std::endl;
        std::cout << "=== All tests passed! ===" << std::endl;
        
    } catch (const exception::EnGeneException& e) {
        std::cerr << "[ERROR] EnGene exception: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Standard exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
