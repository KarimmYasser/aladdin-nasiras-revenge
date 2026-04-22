//
// Created by mohse on 4/18/2026.
//

#pragma once
#include "logger.hpp"
#include "physics-world.hpp"
#include "components/rigid-body.hpp"
#include "components/collider.hpp"
#include "components/aladdin-controller.hpp"
#include "components/movement.hpp"
#include "ecs/world.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace our {
    class PhysicsSystem {
   public:
        bool initialize() {
            return physicsWorld.initialize();
        }

        void update(World* ecsWorld, float deltaTime) {
            if (!ecsWorld) {
                Logger::error("PhysicsSystem", "Cannot update physics system - ECS world is null.");
                return;
            }

            // ====================================================================
            // PHASE 1: Initialize missing bodies/colliders
            // ====================================================================
            for (auto entity : ecsWorld->getEntities()) {
                auto* rbComp = entity->getComponent<RigidBodyComponent>();
                auto* colliderComp = entity->getComponent<ColliderComponent>();
                auto* transform = &entity->localTransform;

                if (rbComp && transform) {
                    // If the rigid body hasn't been created yet, create it
                    if (!rbComp->bodyHandle) {
                        physicsWorld.createRigidBody(entity, rbComp->getDesc());
                        Logger::debug("PhysicsSystem", "Created rigid body for entity '", entity->name, "'");
                    }

                    // If a collider component exists and hasn't been created yet, create it
                    if (colliderComp && !colliderComp->colliderHandle) {
                        physicsWorld.createCollider(entity, colliderComp->getDesc());
                        Logger::debug("PhysicsSystem", "Created collider for entity '", entity->name, "'");
                    }
                }
            }

            // ====================================================================
            // PHASE 2: SYNC ECS -> PHYSICS (Kinematic bodies only)
            // ====================================================================
            // Kinematic bodies are moved by game code, not by physics forces.
            // We must push their new positions into the physics engine BEFORE stepping.
            for (auto entity : ecsWorld->getEntities()) {
                auto* rbComp = entity->getComponent<RigidBodyComponent>();
                auto* transform = &entity->localTransform;

                if (rbComp && rbComp->bodyHandle && transform && rbComp->type == RigidBodyType::Kinematic) {
                    reactphysics3d::Transform rp3dTransform;
                    rp3dTransform.setPosition(reactphysics3d::Vector3(
                        transform->position.x, transform->position.y, transform->position.z
                    ));

                    glm::quat q = glm::quat(transform->rotation);
                    rp3dTransform.setOrientation(reactphysics3d::Quaternion(q.x, q.y, q.z, q.w));

                    rbComp->bodyHandle->setTransform(rp3dTransform);
                }
            }

            // ====================================================================
            // PHASE 2.5: Desired velocity from Movement -> dynamic bodies (before step)
            // ====================================================================
            for (auto entity : ecsWorld->getEntities()) {
                auto* rbComp = entity->getComponent<RigidBodyComponent>();
                auto* mov = entity->getComponent<MovementComponent>();
                if (rbComp && rbComp->bodyHandle && mov && rbComp->type == RigidBodyType::Dynamic) {
                    physicsWorld.setLinearVelocity(entity, mov->linearVelocity);
                }
            }

            // ====================================================================
            // PHASE 3: STEP THE PHYSICS SIMULATION
            // ====================================================================
            constexpr int maxPhysicsStepsPerFrame = 5;
            const float maxAccumulatedTime = fixedDeltaTime * static_cast<float>(maxPhysicsStepsPerFrame);

            accumulator += deltaTime;
            if (accumulator > maxAccumulatedTime) {
                accumulator = maxAccumulatedTime;
            }

            int stepsCount = 0;
            while (accumulator >= fixedDeltaTime && stepsCount < maxPhysicsStepsPerFrame) {
                physicsWorld.step(fixedDeltaTime);
                accumulator -= fixedDeltaTime;
                stepsCount++;
            }
            if (stepsCount > 0) {
                Logger::debug("PhysicsSystem", "Stepped physics ", stepsCount, " times with dt=",
                             fixedDeltaTime, ", remaining accumulator=", accumulator);
            }

            // ====================================================================
            // PHASE 4: SYNC PHYSICS -> ECS (Dynamic bodies only)
            // ====================================================================
            // The physics engine just finished calculating gravity, collisions, and movement.
            // Now we grab those new positions and push them back to the visual Transform
            // components so the Forward Renderer draws them in the correct spot.
            for (auto entity : ecsWorld->getEntities()) {
                auto* rbComp = entity->getComponent<RigidBodyComponent>();
                auto* transform = &entity->localTransform;

                // We only need to sync DYNAMIC bodies. Static and kinematic bodies are handled differently.
                if (rbComp && rbComp->bodyHandle && transform && rbComp->type == RigidBodyType::Dynamic) {
                    // Get the newly calculated transform from ReactPhysics3D
                    const reactphysics3d::Transform& physicsTransform = rbComp->bodyHandle->getTransform();

                    // 1. Sync Position
                    const reactphysics3d::Vector3& pos = physicsTransform.getPosition();
                    transform->position = glm::vec3(pos.x, pos.y, pos.z);

                    // 2. Sync Rotation (Converting RP3D Quaternion back to Euler angles in degrees)
                    const reactphysics3d::Quaternion& rp3dQuat = physicsTransform.getOrientation();

                    // Construct GLM quaternion from RP3D quaternion (w, x, y, z order)
                    glm::quat glmQuat(rp3dQuat.w, rp3dQuat.x, rp3dQuat.y, rp3dQuat.z);

                    // Validate quaternion before conversion
                    if (glm::isnan(glmQuat.w) || glm::isnan(glmQuat.x) || glm::isnan(glmQuat.y) || glm::isnan(glmQuat.z)) {
                        Logger::error("PhysicsSystem", "Invalid quaternion for entity '", entity->name,
                                     "' - NaN component detected");
                        continue;
                    }

                    // Keep Transform rotation in radians (engine convention)
                    transform->rotation = glm::eulerAngles(glmQuat);
                    if(auto* aladdin = entity->getComponent<AladdinControllerComponent>()) {
                        transform->rotation.x = 0.0f;
                        transform->rotation.y = aladdin->facingYaw + AladdinControllerComponent::meshYawVisualOffset;
                        transform->rotation.z = 0.0f;
                    }

                    // 3. Sync Linear Velocity
                    rbComp->velocity = physicsWorld.getLinearVelocity(entity);
                }
            }
        }

        void shutdown(World* ecsWorld = nullptr) {
            // If we have the ECS world, clean up rigid bodies and colliders properly
            // @param ecsWorld Optional ECS world for cleanup. If provided, all physics handles
            //                  cached on ECS components will be cleared before shutting down
            //                  the physics world. If nullptr, physics world shuts down without
            //                  explicit component cleanup.
            if (ecsWorld) {
                Logger::info("PhysicsSystem", "Cleaning up physics bodies and colliders...");
                int bodiesDestroyed = 0;
                int collidersDestroyed = 0;
                for (auto entity : ecsWorld->getEntities()) {
                    auto* rbComp = entity->getComponent<RigidBodyComponent>();
                    auto* colliderComp = entity->getComponent<ColliderComponent>();

                    try {
                        // Note: Can't access destroyRigidBody/destroyCollider directly here;
                        // physics world destruction will clean up native objects. We must
                        // still clear cached component handles so they don't dangle and so
                        // future setup code can recreate physics objects correctly.
                        if (rbComp && rbComp->bodyHandle) {
                            rbComp->bodyHandle = nullptr;
                            bodiesDestroyed++;
                        }

                        if (colliderComp && colliderComp->colliderHandle) {
                            colliderComp->colliderHandle = nullptr;
                            collidersDestroyed++;
                        }
                    } catch (const std::exception& e) {
                        Logger::error("PhysicsSystem", "Error cleaning physics handles for entity '", entity->name,
                                     "': ", e.what());
                    }
                }
                Logger::info("PhysicsSystem", "Cleared ", bodiesDestroyed, " rigid body references and ",
                             collidersDestroyed, " collider references");
            }

            physicsWorld.shutdown();
        }

        PhysicsWorld& getPhysicsWorld() {
            return physicsWorld;
        }
    private:
        PhysicsWorld physicsWorld; ///< The physics world instance that manages the simulation.
        float accumulator{0.0f}; ///< Accumulates time to ensure fixed time step updates.
        const float fixedDeltaTime = 1.0f / 60.0f; ///< The fixed time step for physics updates (60 FPS).


    };
}
