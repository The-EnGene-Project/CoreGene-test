#include <iostream>
#include <cmath>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/cubemap.h>
#include <gl_base/error.h>
#include <3d/camera/perspective_camera.h>
#include <other_genes/environment_mapping.h>
#include <other_genes/3d_shapes/sphere.h>
#include <other_genes/input_handlers/arcball_input_handler.h>

/**
 * @brief Comprehensive integration test for skybox and environment mapping.
 * 
 * This test demonstrates:
 * - Skybox with multiple environment-mapped objects
 * - Arcball camera controls for interactive navigation
 * - Scene graph integration
 * - Stack system interactions (Shader, Transform, Texture)
 * - Multiple objects with different materials
 * 
 * Controls:
 * - Left Mouse Button + Drag: Rotate camera (orbit)
 * - Middle Mouse Button + Drag: Pan camera
 * - Mouse Wheel: Zoom in/out
 * - ESC: Exit
 */

int main() {
    std::cout << "=== Comprehensive Integration Test ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left Mouse + Drag: Rotate camera (orbit)" << std::endl;
    std::cout << "  Middle Mouse + Drag: Pan camera" << std::endl;
    std::cout << "  Mouse Wheel: Zoom in/out" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;
    std::cout << std::endl;
    // Shared pointers for OpenGL resources (created in on_init)
    texture::CubemapPtr cubemap;
    std::shared_ptr<environment::EnvironmentMapping> env_mapping1;
    std::shared_ptr<environment::EnvironmentMapping> env_mapping2;
    std::shared_ptr<environment::EnvironmentMapping> env_mapping3;
    std::shared_ptr<environment::EnvironmentMapping> env_mapping4;
    
    auto* handler = new input::InputHandler();
    std::shared_ptr<arcball::ArcBallInputHandler> arcball_handler;
    
    auto on_init = [&](engene::EnGene& app) {
        // Create procedural cubemap
        std::cout << "[INIT] Creating procedural cubemap..." << std::endl;
        const int face_size = 512;
        std::array<unsigned char*, 6> face_data;
        
        // Generate colorful environment
        for (int i = 0; i < 6; i++) {
            face_data[i] = new unsigned char[face_size * face_size * 3];
            
            for (int y = 0; y < face_size; y++) {
                for (int x = 0; x < face_size; x++) {
                    int idx = (y * face_size + x) * 3;
                    
                    // Create gradient patterns
                    float gradient_x = static_cast<float>(x) / face_size;
                    float gradient_y = static_cast<float>(y) / face_size;
                    
                    unsigned char r = (i == 0 || i == 1) ? static_cast<unsigned char>(255 * gradient_x) : 80;
                    unsigned char g = (i == 2 || i == 3) ? static_cast<unsigned char>(255 * gradient_y) : 80;
                    unsigned char b = (i == 4 || i == 5) ? static_cast<unsigned char>(255 * (1.0f - gradient_x)) : 80;
                    
                    face_data[i][idx + 0] = r;
                    face_data[i][idx + 1] = g;
                    face_data[i][idx + 2] = b;
                }
            }
        }
        
        try {
            // cubemap = texture::Cubemap::Make(face_size, face_size, face_data);
            cubemap = texture::Cubemap::Make("test/skytest.png");
            std::cout << "✓ Cubemap created successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "✗ Failed to create cubemap: " << e.what() << std::endl;
            for (int i = 0; i < 6; i++) {
                delete[] face_data[i];
            }
            throw;
        }
        
        for (int i = 0; i < 6; i++) {
            delete[] face_data[i];
        }

        // Create multiple environment mapping systems with different configurations
        environment::EnvironmentMappingConfig config1;
        config1.cubemap = cubemap;
        config1.mode = environment::MappingMode::REFLECTION;
        config1.reflection_coefficient = 0.6f;
        config1.base_color = glm::vec3(0.8f, 0.2f, 0.2f);
        env_mapping1 = std::make_shared<environment::EnvironmentMapping>(config1);
        
        environment::EnvironmentMappingConfig config2;
        config2.cubemap = cubemap;
        config2.mode = environment::MappingMode::REFRACTION;
        config2.index_of_refraction = 1.52f;
        config2.base_color = glm::vec3(0.2f, 0.8f, 0.2f);
        env_mapping2 = std::make_shared<environment::EnvironmentMapping>(config2);
        
        environment::EnvironmentMappingConfig config3;
        config3.cubemap = cubemap;
        config3.mode = environment::MappingMode::FRESNEL;
        config3.fresnel_power = 2.0f;
        config3.index_of_refraction = 1.33f;
        config3.base_color = glm::vec3(0.2f, 0.2f, 0.8f);
        env_mapping3 = std::make_shared<environment::EnvironmentMapping>(config3);
        
        environment::EnvironmentMappingConfig config4;
        config4.cubemap = cubemap;
        config4.mode = environment::MappingMode::CHROMATIC_DISPERSION;
        config4.ior_rgb = glm::vec3(1.20f, 1.52f, 1.74f);
        config4.base_color = glm::vec3(0.8f, 0.8f, 0.2f);
        env_mapping4 = std::make_shared<environment::EnvironmentMapping>(config4);
        
        std::cout << "✓ Environment mapping systems created" << std::endl;
        std::cout << "[INIT] Setting up scene..." << std::endl;
        
        // Add skybox
        scene::graph()->addNode("skybox")
            .with<component::SkyboxComponent>(cubemap);
        
        // Create sphere geometry
        auto sphere_geom = Sphere::Make(1.0f, 16, 32);  // radius, stacks, slices
        
        // Create four spheres with different materials
        auto& node1 = scene::graph()->addNode("sphere1")
            .with<component::TransformComponent>(
                transform::Transform::Make()->setTranslate(-3.5f, 0.0f, 0.0f))
            .with<component::CubemapComponent>(cubemap, "environmentMap", 0)
            .with<component::ShaderComponent>(env_mapping1->getShader())
            .with<component::GeometryComponent>(sphere_geom);
        
        auto& node2 = scene::graph()->addNode("sphere2")
            .with<component::TransformComponent>(
                transform::Transform::Make()->setTranslate(-1.2f, 0.0f, 0.0f))
            .with<component::CubemapComponent>(cubemap, "environmentMap", 0)
            .with<component::ShaderComponent>(env_mapping2->getShader())
            .with<component::GeometryComponent>(sphere_geom);
        
        auto& node3 = scene::graph()->addNode("sphere3")
            .with<component::TransformComponent>(
                transform::Transform::Make()->setTranslate(1.2f, 0.0f, 0.0f))
            .with<component::CubemapComponent>(cubemap, "environmentMap", 0)
            .with<component::ShaderComponent>(env_mapping3->getShader())
            .with<component::GeometryComponent>(sphere_geom);
        
        auto& node4 = scene::graph()->addNode("sphere4")
            .with<component::TransformComponent>(
                transform::Transform::Make()->setTranslate(3.5f, 0.0f, 0.0f))
            .with<component::CubemapComponent>(cubemap, "environmentMap", 0)
            .with<component::ShaderComponent>(env_mapping4->getShader())
            .with<component::GeometryComponent>(sphere_geom);
        
        std::cout << "✓ Four spheres added to scene" << std::endl;
        std::cout << "  - Sphere 1 (far left): Reflection" << std::endl;
        std::cout << "  - Sphere 2 (left): Refraction" << std::endl;
        std::cout << "  - Sphere 3 (right): Fresnel" << std::endl;
        std::cout << "  - Sphere 4 (far right): Chromatic Dispersion" << std::endl;
        
        // Create camera
        auto camera = component::PerspectiveCamera::Make(60.0f, 0.1f, 100.0f);
        
        // Set initial camera position
        camera->getTransform()->setTranslate(0.0f, 2.0f, 8.0f);
        scene::graph()->setActiveCamera(camera);
        
        std::cout << "✓ Camera created" << std::endl;
        
        // Attach arcball controls
        arcball_handler = arcball::attachArcballTo(*handler);
        
        std::cout << "✓ Arcball controller initialized" << std::endl;
    };

    auto on_update = [&](double dt) {
        // Arcball controller automatically updates camera position
        // No manual camera manipulation needed
    };

    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene::graph()->draw();
        GL_CHECK("render");
    };

    engene::EnGeneConfig config;
    config.title = "Integration Test - Skybox & Environment Mapping";
    config.width = 1024;
    config.height = 768;
    config.clearColor[0] = 0.1f;
    config.clearColor[1] = 0.1f;
    config.clearColor[2] = 0.15f;
    config.clearColor[3] = 1.0f;

    try {
        engene::EnGene app(on_init, on_update, on_render, config, handler);
        std::cout << "\n[RUNNING] Comprehensive integration test" << std::endl;
        std::cout << "Expected: Four spheres with different materials in skybox environment" << std::endl;
        std::cout << "  - All effects should work simultaneously" << std::endl;
        std::cout << "  - Arcball controls should allow interactive camera navigation" << std::endl;
        std::cout << "  - Scene graph and stack systems should integrate correctly" << std::endl;
        app.run();
        
        std::cout << "\n✓ Integration test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
