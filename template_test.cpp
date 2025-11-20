#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <iostream>

int main() {
    std::cout << "=== Test Template ===" << std::endl;

    try {
        auto on_initialize = [](engene::EnGene& app) {
            std::cout << "[INIT] Initializing test..." << std::endl;
            
            // TODO: Add your initialization code here
            
            std::cout << "[INIT] Test initialized!" << std::endl;
        };
        
        auto on_fixed_update = [](double dt) {
            // TODO: Add your fixed update logic here
        };
        
        auto on_render = [](double alpha) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene::graph()->draw();
            GL_CHECK("render");
        };
        
        engene::EnGeneConfig config;
        config.title = "Test Template";
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
