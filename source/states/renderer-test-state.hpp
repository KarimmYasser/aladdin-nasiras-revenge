#pragma once

#include <asset-loader.hpp>
#include <ecs/world.hpp>
#include <components/camera.hpp>
#include <components/mesh-renderer.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <application.hpp>

// This state tests and shows how to use the Forward renderer.
class RendererTestState: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;

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

        cameraController.enter(getApp());

        glm::ivec2 size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
    }

    void onDraw(double deltaTime) override {
        cameraController.update(&world, (float)deltaTime);
        renderer.render(&world, getApp()->getFrameBufferSize());
    }

    void onDestroy() override {
        cameraController.exit();
        renderer.destroy();
        world.clear();
        our::clearAllAssets();
    }
};