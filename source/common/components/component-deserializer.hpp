#pragma once

#include "../ecs/entity.hpp"
#include "camera.hpp"
#include "collider.hpp"
#include "mesh-renderer.hpp"
#include "free-camera-controller.hpp"
#include "movement.hpp"
#include "rigid-body.hpp"
#include "aladdin-controller.hpp"
#include "collectible.hpp"
#include "hazard.hpp"
#include "enemy.hpp"
#include "breakable.hpp"
#include "checkpoint.hpp"
#include "level-exit.hpp"
#include "room-portal.hpp"
#include "light.hpp"
#include "animator-component.hpp"
#include "skinned-mesh-renderer.hpp"

namespace our {

    // Given a json object, this function picks and creates a component in the given entity
    // based on the "type" specified in the json object which is later deserialized from the rest of the json object
    inline void deserializeComponent(const nlohmann::json& data, Entity* entity){
        std::string type = data.value("type", "");
        Component* component = nullptr;
        //TODO: (Req 8) Add an option to deserialize a "MeshRendererComponent" to the following if-else statement
        if(type == CameraComponent::getID()){
            component = entity->addComponent<CameraComponent>();
        } else if (type == FreeCameraControllerComponent::getID()) {
            component = entity->addComponent<FreeCameraControllerComponent>();
        } else if (type == MovementComponent::getID()) {
            component = entity->addComponent<MovementComponent>();
        } else if (type == MeshRendererComponent::getID()) {
            component = entity->addComponent<MeshRendererComponent>();
        } else if (type == RigidBodyComponent::getID()) {
            component = entity->addComponent<RigidBodyComponent>();
        } else if (type == ColliderComponent::getID()) {
            component = entity->addComponent<ColliderComponent>();
        } else if (type == AladdinControllerComponent::getID()) {
            component = entity->addComponent<AladdinControllerComponent>();
        } else if (type == CollectibleComponent::getID()) {
            component = entity->addComponent<CollectibleComponent>();
        } else if (type == HazardComponent::getID()) {
            component = entity->addComponent<HazardComponent>();
        } else if (type == EnemyComponent::getID()) {
            component = entity->addComponent<EnemyComponent>();
        } else if (type == BreakableComponent::getID()) {
            component = entity->addComponent<BreakableComponent>();
        } else if (type == CheckpointComponent::getID()) {
            component = entity->addComponent<CheckpointComponent>();
        } else if (type == LevelExitComponent::getID()) {
            component = entity->addComponent<LevelExitComponent>();
        } else if (type == RoomPortalComponent::getID()) {
            component = entity->addComponent<RoomPortalComponent>();
        } else if (type == LightComponent::getID()) {
            component = entity->addComponent<LightComponent>();
        } else if (type == AnimatorComponent::getID()) {
            component = entity->addComponent<AnimatorComponent>();
        } else if (type == SkinnedMeshRendererComponent::getID()) {
            component = entity->addComponent<SkinnedMeshRendererComponent>();
        }
        if(component) component->deserialize(data);
    }

}
