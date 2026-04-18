//
// Created by mohse on 4/18/2026.
//

#pragma once
#include "glm/vec3.hpp"

namespace our {
    /**
     * @brief Rigid body type enumeration
     *
     * - Static = 0
     * - Dynamic = 1
     * - Kinematic = 2
     */
    enum class RigidBodyType {
        Static,
        Dynamic,
        Kinematic,
    };

    /**
     * @brief Collision shape types for colliders
     */
    enum class ColliderShape {
        Box,
        Sphere,
        Capsule,
    };

    struct RigidBodyDesc {
        RigidBodyType type = RigidBodyType::Dynamic;
        float mass = 1.0f;
        bool useGravity = true;
        bool lockRotation = true;
        glm::vec3 initialVelocity{0.0f};
    };

    struct ColliderDesc {
        ColliderShape shape = ColliderShape::Box;
        glm::vec3 halfExtents{.5f}; // for box
        float radius = .5f;// for sphere/capsule
        float height = 1.8f; // for capsule
        bool isTrigger = false;// doesn't collide physically but detects overlaps
        float friction = .5f;
        float restitution = 0.0f;// bounciness
    };
}