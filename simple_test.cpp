#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <iostream>

int main() {
    std::cout << "=== Simple Triangle Test (Default Shaders) ===" << std::endl;

    try {
        auto on_initialize = [](engene::EnGene& app) {
            std::cout << "[INIT] Creating triangle..." << std::endl;
            
            // Simple triangle with vec4 positions and colors
            std::vector<float> vertices = {
                // positions (x, y, z, w)     // colors (r, g, b, a)
                0.0f,  0.5f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.0f,  // top (red)
               -0.5f, -0.5f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.0f,  // bottom-left (green)
                0.5f, -0.5f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.0f   // bottom-right (blue)
            };
            std::vector<unsigned int> indices = {0, 1, 2};
            
            auto triangle = geometry::Geometry::Make(
                vertices.data(), indices.data(), 
                3, 3,  // 3 vertices, 3 indices
                4,     // 4 floats for position
                {4}    // 4 floats for color
            );
            
            scene::graph()->addNode("Triangle")
                .with<component::GeometryComponent>(triangle);
            
            std::cout << "[INIT] Triangle created!" << std::endl;
        };
        
        auto on_fixed_update = [](double dt) {};
        
        auto on_render = [](double alpha) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene::graph()->draw();
            GL_CHECK("render");
        };
        
        // Use default configuration (default shaders)
        engene::EnGeneConfig config;
        config.title = "Simple Triangle Test";
        config.width = 800;
        config.height = 600;
        
        engene::EnGene app(on_initialize, on_fixed_update, on_render, config);
        app.run();
        
        std::cout << "[TEST] Success!" << std::endl;
        
    } catch (const exception::EnGeneException& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
