//
// Created by mohse on 4/18/2026.
//

#ifndef GFX_LAB_PHYSICS_TYPES_H
#define GFX_LAB_PHYSICS_TYPES_H
#include "glm/vec3.hpp"

namespace our {
    enum class RigidBodyType {
        Static,
        Dynamic,
        Kinematic,
    };

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

#endif //GFX_LAB_PHYSICS_TYPES_H
