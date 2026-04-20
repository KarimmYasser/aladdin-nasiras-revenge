#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/aladdin-controller.hpp>
#include <systems/collectible.hpp>
#include <systems/hazard.hpp>
#include <systems/enemy.hpp>
#include <systems/checkpoint.hpp>
#include <systems/level-exit.hpp>
#include <asset-loader.hpp>

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::AladdinControllerSystem aladdinController;
    our::CollectibleSystem collectibleSystem;
    our::HazardSystem hazardSystem;
    our::EnemySystem enemySystem;
    our::CheckpointSystem checkpointSystem;
    our::LevelExitSystem levelExitSystem;

    void onInitialize() override {
        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if(config.contains("assets")){
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if(config.contains("world")){
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Initialize the player controller system
        aladdinController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
    }

    void onDraw(double deltaTime) override {
        // Check for level transition
        std::string nextScene = levelExitSystem.getNextScene();
        if(!nextScene.empty()){
            levelExitSystem.clearNextScene();
            // Load the new scene config
            std::ifstream file_in(nextScene);
            if(file_in){
                nlohmann::json new_config = nlohmann::json::parse(file_in, nullptr, true, true);
                file_in.close();
                // Update the app config with the new scene
                getApp()->getConfig()["scene"] = new_config["scene"];
                // Reload the play state
                getApp()->changeState("play");
                return;
            } else {
                std::cerr << "Failed to load next scene: " << nextScene << std::endl;
            }
        }

        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        aladdinController.update(&world, (float)deltaTime);
        collectibleSystem.update(&world, (float)deltaTime);
        hazardSystem.update(&world, (float)deltaTime);
        enemySystem.update(&world, (float)deltaTime);
        checkpointSystem.update(&world);
        levelExitSystem.update(&world);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Remove entities marked for deletion at the end of the frame
        world.deleteMarkedEntities();

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    // For Debugging only
    // TODO - Remove or disable in production builds
    void onImmediateGui() override {
        aladdinController.onImmediateGui(&world);
    }
    //////////////////////////////////////////////

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};