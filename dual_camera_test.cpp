/**
 * @file dual_camera_test.cpp
 * @brief Test with asymmetric scene and two switchable cameras
 * 
 * Features:
 * - Asymmetric scene with multiple colored cubes using Cube class
 * - Two cameras at different positions in the scene
 * - Press 'C' to toggle between cameras
 * - ArcBall camera controls (left mouse to orbit, scroll to zoom)
 * - Material system for cube colors
 * - Press ESC to exit
 */

#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <3d/camera/perspective_camera.h>
#include <gl_base/error.h>
#include <gl_base/material.h>
#include <other_genes/3d_shapes/cube.h>
#include <other_genes/input_handlers/arcball_input_handler.h>
#include <iostream>

using namespace engene;

// Camera node names
const char* CAMERA1_NAME = "Camera1";
const char* CAMERA2_NAME = "Camera2";

// Two separate ArcBall controllers, one for each camera
std::shared_ptr<arcball::ArcBallController> arcball1;
std::shared_ptr<arcball::ArcBallController> arcball2;
std::shared_ptr<arcball::ArcBallController> activeArcball;

// Input handler for camera switching with ArcBall integration
class CameraSwitchHandler : public input::InputHandler {
protected:
    void handleKey(GLFWwindow* window, int key, int scancode, int action, int mods) override {
        if (key == GLFW_KEY_C && action == GLFW_PRESS) {
            // Toggle between cameras
            auto currentCamera = scene::graph()->getActiveCamera();
            if (currentCamera) {
                auto currentNode = scene::graph()->getNodeByName(CAMERA1_NAME);
                if (currentNode && currentNode->payload().get<component::PerspectiveCamera>() == currentCamera) {
                    // Switch to camera 2
                    scene::graph()->setActiveCamera(CAMERA2_NAME);
                    activeArcball = arcball2;
                    std::cout << "Switched to Camera 2 (Top View)" << std::endl;
                } else {
                    // Switch to camera 1
                    scene::graph()->setActiveCamera(CAMERA1_NAME);
                    activeArcball = arcball1;
                    std::cout << "Switched to Camera 1 (Side View)" << std::endl;
                }
                
                // Sync the new active arcball with its camera
                if (activeArcball) {
                    activeArcball->syncWithCameraTarget();
                }
                
                // Detach old arcball and attach new one
                if (arcball1 && arcball2) {
                    arcball1->detachFrom(*this);
                    arcball2->detachFrom(*this);
                    activeArcball->attachTo(*this);
                }
            }
        }
    }
    
    // Note: Mouse/scroll handling is now done through the attached arcball controller
    // No need to override these methods anymore
};

int main() {
    try {
        auto handler = std::make_shared<CameraSwitchHandler>();
        
        auto on_initialize = [&](engene::EnGene& app) {
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            
            // Create Camera 1 - Side view position
            scene::graph()->addNode(CAMERA1_NAME)
                .with<component::PerspectiveCamera>(45.0f, 0.1f, 100.0f);
            
            // Position camera 1
            scene::graph()->getNodeByName(CAMERA1_NAME)
                ->payload().get<component::TransformComponent>()
                ->getTransform()->translate(8.0f, 3.0f, 8.0f);
            
            // Create Camera 2 - Top-down view position
            scene::graph()->addNode(CAMERA2_NAME)
                .with<component::PerspectiveCamera>(60.0f, 0.1f, 100.0f);
            
            // Position camera 2
            scene::graph()->getNodeByName(CAMERA2_NAME)
                ->payload().get<component::TransformComponent>()
                ->getTransform()->translate(0.0f, 12.0f, 0.1f);
            
            // Setup ArcBall controller for Camera 1
            arcball1 = arcball::ArcBallController::CreateFromCameraNode(CAMERA1_NAME);
            arcball1->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            arcball1->setSensitivity(0.001f, 0.001f, 0.001f);
            arcball1->setZoomLimits(2.0f, 50.0f);
            
            // Setup ArcBall controller for Camera 2
            arcball2 = arcball::ArcBallController::CreateFromCameraNode(CAMERA2_NAME);
            arcball2->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            arcball2->setSensitivity(0.001f, 0.001f, 0.001f);
            arcball2->setZoomLimits(2.0f, 50.0f);
            
            // Set Camera 1 as active
            scene::graph()->setActiveCamera(CAMERA1_NAME);
            
            // Set the active arcball to camera 1's controller and attach it
            activeArcball = arcball1;
            activeArcball->attachTo(*handler);
            
            // Create asymmetric scene with colored cubes using Cube class and Material system
            
            // Helper lambda to create a cube with material
            auto createCube = [&app](const std::string& name, const glm::vec3& position, 
                                     const glm::vec3& scale, const glm::vec3& color) {
                auto transform = transform::Transform::Make();
                transform->translate(position.x, position.y, position.z);
                transform->scale(scale.x, scale.y, scale.z);
                
                auto material = material::Material::Make(color);
                auto cube = Cube::Make();
                
                scene::graph()->addNode(name)
                    .with<component::TransformComponent>(transform)
                    .with<component::MaterialComponent>(material)
                    .with<component::GeometryComponent>(cube);
            };
            
            // Central cube (red)
            createCube("CentralCube", glm::vec3(0.0f, 0.0f, 0.0f), 
                      glm::vec3(1.0f), glm::vec3(1.0f, 0.2f, 0.2f));
            
            // Left cube (green)
            createCube("LeftCube", glm::vec3(-3.0f, 0.5f, 0.0f), 
                      glm::vec3(0.8f), glm::vec3(0.2f, 1.0f, 0.2f));
            
            // Right cube (blue)
            createCube("RightCube", glm::vec3(3.0f, -0.5f, 1.0f), 
                      glm::vec3(1.2f), glm::vec3(0.2f, 0.2f, 1.0f));
            
            // Front cube (yellow)
            createCube("FrontCube", glm::vec3(0.0f, 1.0f, -2.5f), 
                      glm::vec3(0.6f), glm::vec3(1.0f, 1.0f, 0.2f));
            
            // Back cube (cyan)
            createCube("BackCube", glm::vec3(1.5f, -1.0f, 3.0f), 
                      glm::vec3(0.9f), glm::vec3(0.2f, 1.0f, 1.0f));
            
            // Top cube (magenta)
            createCube("TopCube", glm::vec3(-1.0f, 3.0f, 0.5f), 
                      glm::vec3(0.7f), glm::vec3(1.0f, 0.2f, 1.0f));
            
            // Bottom cube (orange)
            createCube("BottomCube", glm::vec3(2.0f, -2.5f, -1.0f), 
                      glm::vec3(0.5f), glm::vec3(1.0f, 0.6f, 0.2f));
            
            // Configure shader with camera and material
            auto baseShader = app.getBaseShader();
            scene::graph()->getActiveCamera()->bindToShader(baseShader);
            material::stack()->configureShaderDefaults(baseShader);
            baseShader->Bake();
            
            std::cout << "=== Dual Camera Test ===" << std::endl;
            std::cout << "Controls:" << std::endl;
            std::cout << "  'C' - Switch between cameras" << std::endl;
            std::cout << "  Left Mouse - Orbit camera around scene" << std::endl;
            std::cout << "  Scroll Wheel - Zoom in/out" << std::endl;
            std::cout << "  ESC - Exit" << std::endl;
            std::cout << "\nCurrent: Camera 1 (Side View)" << std::endl;
        };
        
        auto on_fixed_update = [](double dt) {
            // Sync active arcball with its camera each frame
            if (activeArcball) {
                activeArcball->syncWithCameraTarget();
            }
        };
        
        auto on_render = [](double alpha) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene::graph()->draw();
            GL_CHECK("render");
        };
        
        engene::EnGeneConfig config;
        config.title = "Dual Camera Test - Press 'C' to Switch";
        config.width = 800;
        config.height = 600;
        
        engene::EnGene app(on_initialize, on_fixed_update, on_render, config, handler.get());
        app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
