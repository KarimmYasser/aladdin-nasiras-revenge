#pragma once

#include <glad/gl.h>

#include "../ecs/world.hpp"
#include "../ecs/entity.hpp"
#include "../components/room-portal.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/movement.hpp"
#include "../components/rigid-body.hpp"
#include "../ecs/transform.hpp"

#include <reactphysics3d/reactphysics3d.h>

#include <glm/glm.hpp>
#include <iostream>

namespace our {

    class RoomPortalSystem {
        float blackoutRemaining = 0.0f;
        Entity* pendingPlayer = nullptr;
        Entity* pendingPortal = nullptr;
        glm::vec3 pendingPos{0.0f};
        float pendingYawY = 0.0f;
        bool pendingSetYaw = true;

        static void teleportPlayer(Entity* player, const glm::vec3& pos, bool setYaw, float yawYRadians) {
            if(!player) return;
            player->localTransform.position = pos;
            if(setYaw){
                player->localTransform.rotation.x = 0.0f;
                player->localTransform.rotation.y = yawYRadians;
                player->localTransform.rotation.z = 0.0f;
            }
            if(auto* mov = player->getComponent<MovementComponent>()){
                mov->linearVelocity = {0.0f, 0.0f, 0.0f};
                mov->angularVelocity = {0.0f, 0.0f, 0.0f};
            }
            if(auto* rb = player->getComponent<RigidBodyComponent>(); rb && rb->bodyHandle){
                reactphysics3d::Transform t;
                t.setPosition(reactphysics3d::Vector3(pos.x, pos.y, pos.z));
                t.setOrientation(Transform::fromEulerAnglesToRP3DQuaternion(player->localTransform.rotation));
                rb->bodyHandle->setTransform(t);
                rb->bodyHandle->setLinearVelocity(reactphysics3d::Vector3(0.0f, 0.0f, 0.0f));
                rb->bodyHandle->setAngularVelocity(reactphysics3d::Vector3(0.0f, 0.0f, 0.0f));
            }
        }

    public:
        bool inBlackout() const { return blackoutRemaining > 0.0f; }

        /// Call at the start of each frame (before gameplay / renderer when not blacking out).
        void update(World* world, float deltaTime) {
            if(!world) return;

            for(auto entity : world->getEntities()){
                auto* portal = entity->getComponent<RoomPortalComponent>();
                if(portal && portal->cooldownTimer > 0.0f){
                    portal->cooldownTimer -= deltaTime;
                    if(portal->cooldownTimer < 0.0f) portal->cooldownTimer = 0.0f;
                }
            }

            if(blackoutRemaining > 0.0f){
                blackoutRemaining -= deltaTime;
                if(blackoutRemaining <= 0.0f){
                    blackoutRemaining = 0.0f;
                    teleportPlayer(pendingPlayer, pendingPos, pendingSetYaw, pendingYawY);
                    if(pendingPortal){
                        if(auto* pc = pendingPortal->getComponent<RoomPortalComponent>()){
                            pc->cooldownTimer = pc->cooldownDuration;
                        }
                    }
                    pendingPlayer = nullptr;
                    pendingPortal = nullptr;
                }
                return;
            }

            Entity* aladdinEntity = nullptr;
            for(auto entity : world->getEntities()){
                if(entity->getComponent<AladdinControllerComponent>()){
                    aladdinEntity = entity;
                    break;
                }
            }
            if(!aladdinEntity) return;

            glm::vec3 playerPos = glm::vec3(aladdinEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));

            for(auto entity : world->getEntities()){
                auto* portal = entity->getComponent<RoomPortalComponent>();
                if(!portal) continue;
                if(portal->cooldownTimer > 0.0f) continue;

                glm::vec3 portalPos = glm::vec3(entity->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));
                if(glm::distance(playerPos, portalPos) < portal->radius){
                    blackoutRemaining = portal->blackoutDuration;
                    pendingPlayer = aladdinEntity;
                    pendingPortal = entity;
                    pendingPos = portal->targetPosition;
                    pendingSetYaw = portal->setYaw;
                    pendingYawY = glm::radians(portal->targetYawDegrees);
                    std::cout << "[RoomPortal] Starting room transition (" << portal->blackoutDuration << "s)\n";
                    return;
                }
            }
        }

        /// Full-window black clear while `inBlackout()` is true.
        static void renderBlackFrame(const glm::ivec2& framebufferSize) {
            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
        }
    };

}
