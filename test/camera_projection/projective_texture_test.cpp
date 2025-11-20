#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <3d/camera/perspective_camera.h>
#include <3d/lights/point_light.h>
#include <gl_base/transform.h>
#include <other_genes/3d_shapes/cube.h>
#include <other_genes/input_handlers/arcball_input_handler.h>

/**
 * @brief Projective texture mapping test.
 * 
 * This test demonstrates projective texture mapping, where a texture is
 * projected onto geometry from a specific viewpoint (like a slide projector).
 * 
 * The scene contains:
 * - A rotating plane that receives the projected texture
 * - A point light for basic illumination
 * - A projector matrix that defines the projection frustum
 * 
 * The projected texture will appear on surfaces visible from the projector's
 * viewpoint, creating effects like:
 * - Slide projection
 * - Spotlight cookies/gobos
 * - Shadow mapping (with depth comparison)
 * - Decals
 */

// Helper to create a plane geometry
geometry::GeometryPtr createPlane(float size) {
    std::vector<float> vertices = {
        // positions          // normals           // texcoords
        -size, 0.0f, -size,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         size, 0.0f, -size,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         size, 0.0f,  size,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -size, 0.0f,  size,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };
    
    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };
    
    return geometry::Geometry::Make(
        vertices.data(), indices.data(),
        4, 6,
        3, {3, 2}
    );
}

// Use ready Cube implementation from other_genes

int main() {
    std::cout << "=== Projective Texture Mapping Test ===" << std::endl;
    std::cout << "This test demonstrates texture projection onto geometry." << std::endl;
    std::cout << "Watch the checkerboard pattern project onto the rotating objects." << std::endl;
    std::cout << std::endl;

    // Projector matrix (will be updated in on_initialize)
    glm::mat4 projectorMatrix;
    
    // Create a platform InputHandler and an arcball handler placeholder
    auto* handler = new input::InputHandler();
    std::shared_ptr<arcball::ArcBallController> arcball_handler;

    auto on_initialize = [&](engene::EnGene& app) {
        glEnable(GL_DEPTH_TEST);
        
        // Create shader by passing file paths
        auto projectiveShader = shader::Shader::Make(
            std::string("core_gene/shaders/projective_texture_vertex.glsl"),
            std::string("core_gene/shaders/projective_texture_fragment.glsl")
        );
        
        // Configure uniforms
        projectiveShader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
        projectiveShader->configureDynamicUniform<uniform::detail::Sampler>(
            "u_projectedTexture", 
            texture::getSamplerProvider("u_projectedTexture")
        );
        
        // Create a checkerboard texture for projection
        const int texSize = 256;
        std::vector<unsigned char> checkerboard(texSize * texSize * 4);
        for (int y = 0; y < texSize; ++y) {
            for (int x = 0; x < texSize; ++x) {
                int idx = (y * texSize + x) * 4;
                bool isWhite = ((x / 32) + (y / 32)) % 2 == 0;
                unsigned char color = isWhite ? 255 : 64;
                checkerboard[idx + 0] = color;
                checkerboard[idx + 1] = color;
                checkerboard[idx + 2] = color;
                checkerboard[idx + 3] = 255;
            }
        }
        auto projectedTexture = texture::Texture::Make(texSize, texSize, checkerboard.data());
        
        // Setup projector matrix (view-projection from projector's perspective)
        glm::vec3 projectorPos(0.0f, 5.0f, 5.0f);
        glm::vec3 projectorTarget(0.0f, 0.0f, 0.0f);
        glm::vec3 projectorUp(0.0f, 1.0f, 0.0f);
        
        glm::mat4 projectorView = glm::lookAt(projectorPos, projectorTarget, projectorUp);
        glm::mat4 projectorProj = glm::perspective(glm::radians(60.0f), 1.0f, 1.0f, 20.0f);
        projectorMatrix = projectorProj * projectorView;
        
        projectiveShader->setUniform<glm::mat4>("u_projectorMatrix", projectorMatrix);
        projectiveShader->setUniform<float>("u_projectionIntensity", 0.8f);
        
        // Create geometries
        auto plane = createPlane(5.0f);
        auto cube = Cube::Make(1.5f, 1.5f, 1.5f);

        // Create a small marker cube at the projector position so we can visualize the projector
        auto projectorMarker = Cube::Make(0.2f, 0.2f, 0.2f);
        auto projectorMarkerTransform = transform::Transform::Make();
        projectorMarkerTransform->translate(projectorPos.x, projectorPos.y, projectorPos.z);
        auto projectorMarkerMaterial = material::Material::Make(glm::vec3(1.0f, 0.2f, 0.2f));
        scene::graph()->addNode("ProjectorMarker")
            .with<component::TransformComponent>(projectorMarkerTransform)
            .with<component::MaterialComponent>(projectorMarkerMaterial)
            .with<component::GeometryComponent>(projectorMarker);
        
        // Setup camera
        scene::graph()->addNode("CameraNode")
            .with<component::PerspectiveCamera>();
        scene::graph()->setActiveCamera("CameraNode");
        scene::graph()->getNodeByName("CameraNode")
            ->payload().get<component::TransformComponent>()
            ->getTransform()->translate(0, 4, 10);
        
        // Create light
        auto light_transform = transform::Transform::Make();
        light_transform->translate(3, 5, 3);
        light::PointLightParams params;
        params.diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        params.specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        params.ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        auto light = light::PointLight::Make(params);
        
        scene::graph()->addNode("LightNode")
            .with<component::TransformComponent>(light_transform)
            .with<component::LightComponent>(light, light_transform);
        
        // Create plane (ground)
        auto plane_transform = transform::Transform::Make();
        plane_transform->translate(0, -1, 0);
        
        scene::graph()->addNode("Plane")
            .with<component::TransformComponent>(plane_transform)
            .with<component::ShaderComponent>(projectiveShader)
            .with<component::TextureComponent>(projectedTexture, "u_projectedTexture", 0)
            .with<component::GeometryComponent>(plane);
        
        // Create rotating cube
        auto cube_transform = transform::Transform::Make();
        cube_transform->translate(0, 1, 0);
        
        scene::graph()->addNode("Cube")
            .with<component::TransformComponent>(cube_transform)
            .with<component::ShaderComponent>(projectiveShader)
            .with<component::TextureComponent>(projectedTexture, "u_projectedTexture", 0)
            .with<component::GeometryComponent>(cube);
        
        // Bind light and camera to shader
        light::manager().bindToShader(projectiveShader);
        scene::graph()->getActiveCamera()->bindToShader(projectiveShader);
        projectiveShader->Bake();
        light::manager().apply();
        
        scene::graph()->getActiveCamera()->setAspectRatio(1.0f);
        
        std::cout << "✓ Scene initialized" << std::endl;
        std::cout << "  - Projector positioned at (0, 5, 5) looking at origin" << std::endl;
        std::cout << "  - Checkerboard texture will be projected onto objects" << std::endl;

        // Attach arcball controls to the input handler so user can inspect the scene
        arcball_handler = arcball::attachArcballTo(*handler);
        std::cout << "✓ Arcball controller attached" << std::endl;
    };
    
    auto on_fixed_update = [](double dt) {
        // Rotate the cube
        if (auto cube = scene::graph()->getNodeByName("Cube")) {
            auto transform_component = cube->payload().get<component::TransformComponent>();
            if (transform_component) {
                transform_component->getTransform()->rotate(20.0f * (float)dt, 0, 1, 0);
                transform_component->getTransform()->rotate(15.0f * (float)dt, 1, 0, 0);
            }
        }
    };
    
    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene::graph()->draw();
        GL_CHECK("render");
    };
    
    engene::EnGeneConfig config;
    config.title = "Projective Texture Mapping Test";
    config.width = 800;
    config.height = 800;
    config.clearColor[0] = 0.1f;
    config.clearColor[1] = 0.1f;
    config.clearColor[2] = 0.15f;
    config.clearColor[3] = 1.0f;
    
    try {
        engene::EnGene app(on_initialize, on_fixed_update, on_render, config, handler);
        std::cout << "\n[RUNNING] Projective texture test" << std::endl;
        std::cout << "Expected: Checkerboard pattern projected onto plane and rotating cube" << std::endl;
        app.run();
        
        std::cout << "\n✓ Test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
