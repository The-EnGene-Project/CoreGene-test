#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <other_genes/3d_shapes/cube.h>
#include <iostream>

class WASDController : public input::InputHandler {
private:
    scene::SceneNodePtr controlled_node;
    transform::TransformPtr node_transform;
    float pos_x, pos_y, pos_z, speed;

protected:
    void moveForward() {
        pos_z -= speed;
    }
    void moveBackward() {
        pos_z += speed;
    }
    void moveLeft() {
        pos_x -= speed;
    }
    void moveRight() {
        pos_x += speed;
    }
    void moveUp() {
        pos_y += speed;
    }
    void moveDown() {
        pos_y -= speed;
    }
public:
    WASDController() : pos_x(0.0f), pos_y(0.0f), pos_z(0.0f), speed(0.02f) {}

    void setSpeed(float new_speed) {
        speed = new_speed;
    }

    void handleKey(KEY_HANDLER_ARGS) override {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            switch (key)
            {
            case GLFW_KEY_Q:
                glfwSetWindowShouldClose(window, GLFW_TRUE); break;
            case GLFW_KEY_W:
                moveForward(); break;
            case GLFW_KEY_S:
                moveBackward(); break;
            case GLFW_KEY_A:
                moveLeft(); break;
            case GLFW_KEY_D:
                moveRight(); break;
            case GLFW_KEY_Z:
                moveUp(); break;
            case GLFW_KEY_X:
                moveDown(); break;
            
            default:
                break;
            }
        }
    }

    void setControlledNode(const scene::SceneNodePtr& node) {
        controlled_node = node;
        std::cout << "Controlled node set to: " << controlled_node->getName() << std::endl;
        node_transform = controlled_node->payload().get<component::ObservedTransformComponent>()->getTransform();
    }

    void setTransform(const transform::TransformPtr& transform) {
        node_transform = transform;
    }

    void animate() {
        if (node_transform) {
            node_transform->setTranslate(pos_x, pos_y, pos_z);
        }
    }
};

int main() {
    std::cout << "=== Test Template ===" << std::endl;

    auto handler = new WASDController();

    try {
        auto on_initialize = [&](engene::EnGene& app) {
            std::cout << "[INIT] Initializing test..." << std::endl;
            
            // TODO: Add your initialization code here
            scene::graph()->addNode("player")
                .with<component::ObservedTransformComponent>(
                    transform::Transform::Make(),
                    "player_transform"
                )
                .with<component::GeometryComponent>(
                    // Placeholder geometry, replace with actual geometry
                    Cube::Make(),
                    "player_geometry"
                );
            
            handler->setTransform(scene::graph()->getNodeByName("player")
            ->payload().get<component::ObservedTransformComponent>("player_transform")->getTransform());
                
            
            std::cout << "[INIT] Test initialized!" << std::endl;
        };
        
        auto on_fixed_update = [&](double dt) {
            // TODO: Add your fixed update logic here
        };
        
        auto on_render = [&](double alpha) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            handler->animate();
            scene::graph()->draw();
            GL_CHECK("render");
        };
        
        engene::EnGeneConfig config;
        config.title = "Test Template";
        config.width = 800;
        config.height = 600;
        
        engene::EnGene app(on_initialize, on_fixed_update, on_render, config, handler);
        app.run();
        
        std::cout << "[TEST] Success!" << std::endl;
        
    } catch (const exception::EnGeneException& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
