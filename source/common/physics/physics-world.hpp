// Created by mohse on 4/18/2026.
//

#pragma once
#include "glm/vec3.hpp"
#include <physics/physics-types.hpp>
#include <vector>

#include "ecs/entity.hpp"
#include "reactphysics3d/engine/PhysicsCommon.h"

namespace reactphysics3d {
    class EventListener;
}

namespace our {
    /**
     * @brief Represents the result of a raycast in the physics world.
     */
    struct RaycastHit {
        bool hasHit = false;       ///< Indicates whether the ray hit an object.
        Entity* entity{nullptr}; ///< Pointer to the entity that was hit
        glm::vec3 point{0.0f};     ///< The point of intersection in world space.
        glm::vec3 normal{0.0f};    ///< The surface normal at the point of intersection.
        float distance = 0.0f;     ///< The distance from the ray's origin to the hit point.
    };

    enum class PhysicsEventType {
        Trigger,
        Contact
    };

    enum class PhysicsEventPhase {
        Begin,
        Stay,
        End
    };

    struct PhysicsEvent {
        PhysicsEventType type = PhysicsEventType::Contact;
        PhysicsEventPhase phase = PhysicsEventPhase::Begin;
        Entity* entityA = nullptr;
        Entity* entityB = nullptr;
        glm::vec3 normal{0.0f};
        float penetrationDepth = 0.0f;
    };

    class Entity;

    /**
     * @brief Manages the physics simulation for the game world.
     *
     * This class wraps the ReactPhysics3D library to provide functionality for
     * creating rigid bodies, colliders, performing raycasts, and stepping the
     * physics simulation forward in time.
     */
    class PhysicsWorld {
    private:
        friend class PhysicsWorldEventListener;

        bool initialized = false; ///< Indicates whether the physics world has been initialized.

        reactphysics3d::PhysicsCommon physicsCommon; ///< Manages memory and resources for the physics engine.
        reactphysics3d::PhysicsWorld* physicsWorld{nullptr}; ///< Pointer to the physics world instance.
        reactphysics3d::EventListener* eventListener{nullptr};

        std::vector<PhysicsEvent> contactEvents;
        std::vector<PhysicsEvent> triggerEvents;

    public:
        /**
         * @brief Initializes the physics world.
         *
         * @return True if initialization was successful, false otherwise.
         */
        bool initialize();

        /**
         * @brief Shuts down the physics world and releases resources.
         */
        void shutdown();

        /**
         * @brief Advances the physics simulation by a given time step.
         *
         * @param dt The time step (in seconds) to advance the simulation.
         */
        void step(float dt) const;

        /**
         * @brief Creates a rigid body in the physics world.
         *
         * @param entity The entity associated with the rigid body.
         * @param desc The description of the rigid body (e.g., mass, position, etc.).
         */
        void createRigidBody(Entity* entity, const RigidBodyDesc& desc);
 
        /**
         * @brief Destroys a rigid body in the physics world.
         * 
         * @param entity The entity associated with the rigid body to destroy.
         */
        void destroyRigidBody(Entity* entity);

        /**
         * @brief Creates a collider and attaches it to a rigid body.
         *
         * @param entity The entity associated with the collider.
         * @param desc The description of the collider (e.g., shape, size, etc.).
         */
        void createCollider(Entity* entity, const ColliderDesc& desc);

        /**
         * @brief Sets the linear velocity of a rigid body.
         *
         * @param entity The entity associated with the rigid body.
         * @param velocity The new linear velocity to set.
         */
        void setLinearVelocity(Entity* entity, const glm::vec3& velocity) const;

        /**
         * @brief Gets the linear velocity of a rigid body.
         *
         * @param entity The entity associated with the rigid body.
         * @return The current linear velocity of the rigid body.
         */
        glm::vec3 getLinearVelocity(Entity* entity) const;

        /**
         * @brief Performs a raycast in the physics world.
         *
         * @param origin The starting point of the ray.
         * @param direction The direction of the ray (should be normalized).
         * @param maxDistance The maximum distance the ray can travel.
         * @return A RaycastHit structure containing the result of the raycast.
         */
        [[nodiscard]] RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const;

        void clearFrameEvents();

        [[nodiscard]] const std::vector<PhysicsEvent>& getContactEvents() const {
            return contactEvents;
        }

        [[nodiscard]] const std::vector<PhysicsEvent>& getTriggerEvents() const {
            return triggerEvents;
        }

        [[nodiscard]] bool hasContactEvent(Entity* a, Entity* b, bool includeStay = true) const;
        [[nodiscard]] bool hasTriggerEvent(Entity* a, Entity* b, bool includeStay = true) const;
        [[nodiscard]] bool hasAnyInteraction(Entity* a, Entity* b, bool includeStay = true) const;
        [[nodiscard]] bool testOverlap(Entity* a, Entity* b) const;
        [[nodiscard]] bool isGrounded(Entity* entity, float minUpDot = 0.5f) const;
    };

}
