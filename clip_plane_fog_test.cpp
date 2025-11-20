#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <3d/camera/perspective_camera.h>
#include <3d/lights/directional_light.h>
#include <3d/lights/point_light.h>
#include <3d/lights/spot_light.h>
#include <other_genes/3d_shapes/sphere.h>
#include <other_genes/input_handlers/arcball_input_handler.h>
#include <gl_base/uniforms/uniform.h>

/**
 * @brief Comprehensive test for clip planes and fog with multiple lights.
 * 
 * This test demonstrates:
 * - Multiple clip planes cutting through geometry
 * - Exponential fog effect
 * - Multiple lights (directional, point, spot)
 * - Arcball camera controls
 * - Scene graph integration
 * 
 * Controls:
 * - Left Mouse Button + Drag: Rotate camera (orbit)
 * - Middle Mouse Button + Drag: Pan camera
 * - Mouse Wheel: Zoom in/out
 * - ESC: Exit
 */

int main() {
    std::cout << "=== Clip Plane & Fog Test ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left Mouse + Drag: Rotate camera (orbit)" << std::endl;
    std::cout << "  Middle Mouse + Drag: Pan camera" << std::endl;
    std::cout << "  Mouse Wheel: Zoom in/out" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;
    std::cout << std::endl;

    auto* handler = new input::InputHandler();
    std::shared_ptr<arcball::ArcBallInputHandler> arcball_handler;
    
    // Shared shader pointer
    shader::ShaderPtr fog_shader;
    
    auto on_init = [&](engene::EnGene& app) {
        std::cout << "[INIT] Setting up lights first..." << std::endl;
        
        // Create directional light
        light::DirectionalLightParams dir_params;
        dir_params.base_direction = glm::vec3(-0.5f, -1.0f, -0.3f);
        dir_params.ambient = glm::vec4(0.2f, 0.2f, 0.25f, 1.0f);
        dir_params.diffuse = glm::vec4(0.6f, 0.6f, 0.7f, 1.0f);
        dir_params.specular = glm::vec4(0.3f, 0.3f, 0.4f, 1.0f);
        auto directional_light = light::DirectionalLight::Make(dir_params);
        auto dir_transform = transform::Transform::Make();
        
        scene::graph()->addNode("dir_light")
            .with<component::LightComponent>(directional_light, dir_transform);
        
        // Create red point light (left)
        light::PointLightParams point1_params;
        point1_params.position = glm::vec4(-5.0f, 3.0f, 0.0f, 1.0f);
        point1_params.ambient = glm::vec4(0.1f, 0.0f, 0.0f, 1.0f);
        point1_params.diffuse = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
        point1_params.specular = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
        point1_params.constant = 1.0f;
        point1_params.linear = 0.09f;
        point1_params.quadratic = 0.032f;
        auto point_light1 = light::PointLight::Make(point1_params);
        auto point1_transform = transform::Transform::Make();
        
        scene::graph()->addNode("point_light1")
            .with<component::LightComponent>(point_light1, point1_transform);
        
        // Create green point light (right)
        light::PointLightParams point2_params;
        point2_params.position = glm::vec4(5.0f, 3.0f, 0.0f, 1.0f);
        point2_params.ambient = glm::vec4(0.0f, 0.1f, 0.0f, 1.0f);
        point2_params.diffuse = glm::vec4(0.2f, 1.0f, 0.2f, 1.0f);
        point2_params.specular = glm::vec4(0.5f, 1.0f, 0.5f, 1.0f);
        point2_params.constant = 1.0f;
        point2_params.linear = 0.09f;
        point2_params.quadratic = 0.032f;
        auto point_light2 = light::PointLight::Make(point2_params);
        auto point2_transform = transform::Transform::Make();
        
        scene::graph()->addNode("point_light2")
            .with<component::LightComponent>(point_light2, point2_transform);
        
        // Create blue spot light (top)
        light::SpotLightParams spot_params;
        spot_params.position = glm::vec4(0.0f, 8.0f, 0.0f, 1.0f);
        spot_params.base_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        spot_params.ambient = glm::vec4(0.0f, 0.0f, 0.1f, 1.0f);
        spot_params.diffuse = glm::vec4(0.3f, 0.3f, 1.0f, 1.0f);
        spot_params.specular = glm::vec4(0.6f, 0.6f, 1.0f, 1.0f);
        spot_params.constant = 1.0f;
        spot_params.linear = 0.045f;
        spot_params.quadratic = 0.0075f;
        spot_params.cutOff = glm::cos(glm::radians(25.0f));
        auto spot_light = light::SpotLight::Make(spot_params);
        auto spot_transform = transform::Transform::Make();
        
        scene::graph()->addNode("spot_light")
            .with<component::LightComponent>(spot_light, spot_transform);
        
        std::cout << "✓ Four lights created (1 directional, 2 point, 1 spot)" << std::endl;
        
        std::cout << "[INIT] Creating custom shader with fog and clip planes..." << std::endl;
        
        // Load custom shaders from files (AFTER lights are created so SceneLights UBO exists)
        try {
            
            fog_shader = shader::Shader::Make(
                "core_gene/shaders/clip_plane_vertex.glsl", 
                "core_gene/shaders/fragment_fog.glsl"
            );
            
            // Configure uniform blocks
            fog_shader->addResourceBlockToBind("CameraMatrices");
            fog_shader->addResourceBlockToBind("CameraPosition");
            fog_shader->addResourceBlockToBind("SceneLights");
            
            // Configure dynamic uniforms
            fog_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            
            // Configure material uniforms from material stack
            material::stack()->configureShaderDefaults(fog_shader);
            
            fog_shader->Bake();
            std::cout << "✓ Custom shader compiled and linked" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "✗ Shader compilation failed: " << e.what() << std::endl;
            throw;
        }
        
        std::cout << "[INIT] Setting up scene with clip planes and fog..." << std::endl;
        
        // Create sphere geometry
        auto sphere_geom = Sphere::Make(1.0f, 32, 64);
        
        // Create a base material for all spheres
        auto base_material = material::Material::Make(glm::vec3(0.8f, 0.8f, 0.8f));
        base_material->setShininess(64.0f);
        
        // Create fog uniforms (these are scene-wide, not per-material)
        auto fog_vars = component::VariableComponent::Make(
            uniform::Uniform<glm::vec3>::Make("fogcolor", []() { 
                return glm::vec3(0.5f, 0.6f, 0.7f); // Blueish fog
            })
        );
        fog_vars->addUniform(uniform::Uniform<float>::Make("fogdensity", []() { 
            return 0.08f; // Moderate fog density
        }));
        
        // Texture flags (these are also scene-wide for this test)
        fog_vars->addUniform(uniform::Uniform<bool>::Make("u_hasNormalMap", []() { return false; }));
        fog_vars->addUniform(uniform::Uniform<bool>::Make("u_hasRoughnessMap", []() { return false; }));
        fog_vars->addUniform(uniform::Uniform<bool>::Make("u_hasDiffuseMap", []() { return false; }));
        
        // Center sphere with two clip planes
        auto center_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");
        center_clip->addPlane(1.0f, 0.0f, 0.0f, 0.0f);  // Cut along X axis
        center_clip->addPlane(0.0f, 1.0f, 0.0f, 0.0f);  // Cut along Y axis
        
        scene::graph()->addNode("center_sphere")
            .with<component::TransformComponent>(
                transform::Transform::Make()->translate(0.0f, 0.0f, 0.0f)->scale(2.0f, 2.0f, 2.0f))
            .with<component::ShaderComponent>(fog_shader)
            .with<component::MaterialComponent>(base_material)
            .addComponent(fog_vars)
            .addComponent(center_clip)
            .addComponent(component::GeometryComponent::Make(sphere_geom));
        
        // Left sphere - single clip plane
        auto left_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");
        left_clip->addPlane(0.0f, 0.0f, 1.0f, 0.0f);  // Cut along Z axis
        
        scene::graph()->addNode("left_sphere")
            .with<component::TransformComponent>(
                transform::Transform::Make()->translate(-4.0f, 0.0f, 0.0f)->scale(1.5f, 1.5f, 1.5f))
            .with<component::ShaderComponent>(fog_shader)
            .with<component::MaterialComponent>(base_material)
            .addComponent(fog_vars)
            .addComponent(left_clip)
            .addComponent(component::GeometryComponent::Make(sphere_geom));
        
        // Right sphere - no clip planes
        auto right_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");
        // Don't add any planes - will automatically set num_clip_planes = 0
        
        scene::graph()->addNode("right_sphere")
            .with<component::TransformComponent>(
                transform::Transform::Make()->translate(4.0f, 0.0f, 0.0f)->scale(1.5f, 1.5f, 1.5f))
            .with<component::ShaderComponent>(fog_shader)
            .with<component::MaterialComponent>(base_material)
            .addComponent(fog_vars)
            .addComponent(right_clip)
            .addComponent(component::GeometryComponent::Make(sphere_geom));
        
        // Add some distant spheres to show fog effect
        for (int i = 0; i < 5; i++) {
            float z = -5.0f - (i * 3.0f);
            
            // ClipPlaneComponent with no planes for distant spheres
            auto fog_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");
            
            scene::graph()->addNode("fog_sphere_" + std::to_string(i))
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(0.0f, 0.0f, z)->scale(1.0f, 1.0f, 1.0f))
                .with<component::ShaderComponent>(fog_shader)
                .with<component::MaterialComponent>(base_material)
                .addComponent(fog_vars)
                .addComponent(fog_clip)
                .addComponent(component::GeometryComponent::Make(sphere_geom));
        }
        
        std::cout << "✓ Scene created with 8 spheres" << std::endl;
        std::cout << "  - Center: 2 clip planes (X and Y)" << std::endl;
        std::cout << "  - Left: 1 clip plane (Z)" << std::endl;
        std::cout << "  - Right: No clip planes" << std::endl;
        std::cout << "  - 5 distant spheres to demonstrate fog" << std::endl;
        
        // Create camera
        auto camera = component::PerspectiveCamera::Make(60.0f, 1.0f, 1000.0f);
        // camera->getTransform()->translate(0.0f, 3.0f, 12.0f);
        scene::graph()->setActiveCamera(camera);

        light::manager().apply();
        
        std::cout << "✓ Camera created" << std::endl;
        
        // Attach arcball controls
        arcball_handler = arcball::attachArcballTo(*handler);
        
        std::cout << "✓ Arcball controller initialized" << std::endl;
    };

    auto on_update = [&](double dt) {
        // Arcball controller automatically updates camera
    };

    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene::graph()->draw();
        GL_CHECK("render");
    };

    engene::EnGeneConfig config;
    config.title = "Clip Plane & Fog Test - Multiple Lights";
    config.width = 1280;
    config.height = 720;
    config.clearColor[0] = 0.5f;
    config.clearColor[1] = 0.6f;
    config.clearColor[2] = 0.7f;
    config.clearColor[3] = 1.0f;

    try {
        engene::EnGene app(on_init, on_update, on_render, config, handler);
        std::cout << "\n[RUNNING] Clip plane and fog test" << std::endl;
        std::cout << "Expected:" << std::endl;
        std::cout << "  - Center sphere cut by two planes" << std::endl;
        std::cout << "  - Left sphere cut by one plane" << std::endl;
        std::cout << "  - Right sphere intact" << std::endl;
        std::cout << "  - Distant spheres fade into fog" << std::endl;
        std::cout << "  - Multiple colored lights illuminating the scene" << std::endl;
        app.run();
        
        std::cout << "\n✓ Test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
