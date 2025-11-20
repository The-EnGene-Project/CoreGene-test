#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <3d/camera/perspective_camera.h>
#include <3d/lights/point_light.h>
#include <gl_base/transform.h>

// Helper function to read shader source from a file
std::string readShaderSource(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to create a sphere geometry
geometry::GeometryPtr createSphere(float radius, int sectors, int stacks) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;

    float sectorStep = 2 * PI / sectors;
    float stackStep = PI / stacks;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stacks; ++i) {
        stackAngle = PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for(int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;

            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);

            s = (float)j / sectors;
            t = (float)i / stacks;
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    int k1, k2;
    for(int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    return geometry::Geometry::Make(
        vertices.data(), indices.data(),
        vertices.size() / 8, indices.size(),
        3, {3, 2}
    );
}


int main() {
    try {
        auto on_initialize = [](engene::EnGene& app) {
            // Enable depth testing
            glEnable(GL_DEPTH_TEST);
            
            // Load shaders
            auto vs_source = readShaderSource("core_gene/shaders/specular_gloss_vertex.glsl");
            auto fs_source = readShaderSource("core_gene/shaders/specular_gloss_fragment.glsl");
            auto specularShader = shader::Shader::Make(vs_source, fs_source);
            
            specularShader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            specularShader->configureDynamicUniform<uniform::detail::Sampler>("u_specularMap", texture::getSamplerProvider("u_specularMap"));
            specularShader->configureDynamicUniform<uniform::detail::Sampler>("u_glossMap", texture::getSamplerProvider("u_glossMap"));

            // Create textures programmatically
            unsigned char whitePixel[] = { 255, 255, 255, 255 };
            auto specularMap = texture::Texture::Make(1, 1, whitePixel);

            // Create a 2x2 gloss texture with varied values to visualize the effect
            // Layout: [black, dark grey]
            //         [light grey, white]
            unsigned char glossPixels[] = {
                0, 0, 0, 255,       // Bottom-left: black (no gloss)
                85, 85, 85, 255,    // Bottom-right: dark grey (low gloss)
                170, 170, 170, 255, // Top-left: light grey (medium gloss)
                255, 255, 255, 255  // Top-right: white (high gloss)
            };
            auto glossMap = texture::Texture::Make(2, 2, glossPixels);

            // Create sphere
            auto sphere = createSphere(1.0f, 32, 16);

            // Build Scene
            scene::graph()->addNode("CameraNode")
                .with<component::PerspectiveCamera>();

            scene::graph()->setActiveCamera("CameraNode");
            // Position camera higher and further back to see the orbit better
            scene::graph()->getNodeByName("CameraNode")->payload().get<component::TransformComponent>()->getTransform()->translate(0, 3, 8);

            // Create light with proper parameters
            auto light_transform = transform::Transform::Make();
            light_transform->translate(2, 2, 2);
            light::PointLightParams params;
            params.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            params.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            params.ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
            auto light = light::PointLight::Make(params);

            scene::graph()->addNode("LightNode")
                .with<component::TransformComponent>(light_transform)
                .with<component::LightComponent>(light, light_transform);

            // Create orbit node that will rotate around the origin
            auto orbit_transform = transform::Transform::Make();
            
            scene::graph()->addNode("OrbitNode")
                .with<component::TransformComponent>(orbit_transform);
            
            // Create sphere as a child of the orbit node, translated outward
            auto sphere_transform = transform::Transform::Make();
            sphere_transform->translate(3, 0, 0);  // Translate sphere 3 units on X axis
            
            scene::graph()->buildAt("OrbitNode")
                .addNode("Sphere")
                    .with<component::TransformComponent>(sphere_transform)
                    .with<component::ShaderComponent>(specularShader)
                    .with<component::TextureComponent>(specularMap, "u_specularMap", 0)
                    .with<component::TextureComponent>(glossMap, "u_glossMap", 1)
                    .with<component::GeometryComponent>(sphere)
            ;

            light::manager().bindToShader(specularShader);
            scene::graph()->getActiveCamera()->bindToShader(specularShader);
            specularShader->Bake();

            light::manager().apply();

            scene::graph()->getActiveCamera()->setAspectRatio(1.0f);
        };

        auto on_fixed_update = [](double dt) {
            // Rotate the orbit node to make the sphere orbit around the center
            if (auto orbit = scene::graph()->getNodeByName("OrbitNode")) {
                auto transform_component = orbit->payload().get<component::TransformComponent>();
                if (transform_component) {
                    // Rotate 30 degrees per second around Y axis
                    transform_component->getTransform()->rotate(30.0f * (float)dt, 0, 1, 0);
                }
            }
        };

        auto on_render = [](double alpha) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene::graph()->draw();
            GL_CHECK("render");
        };

        engene::EnGeneConfig config;
        config.title = "Specular Gloss Test";
        config.width = 800;
        config.height = 800;

        engene::EnGene app(on_initialize, on_fixed_update, on_render, config);
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
