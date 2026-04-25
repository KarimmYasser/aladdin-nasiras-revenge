//
// Created by mohse on 4/18/2026.
//

#pragma once
#include "glm/vec3.hpp"
#include <string>

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
        /// Static triangle soup (ReactPhysics3D ConcaveMeshShape). Requires a loaded Mesh asset with cook data.
        ConcaveMesh,
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
        /// Box/sphere/capsule center in rigid-body local space (matches mesh scale frame: T*R*S).
        glm::vec3 centerOffset{0.0f};
        float radius = .5f;// for sphere/capsule
        float height = 1.8f; // for capsule
        bool isTrigger = false;// doesn't collide physically but detects overlaps
        float friction = .5f;
        float restitution = 0.0f;// bounciness
        /// AssetLoader<Mesh> key — mesh must have been loaded from OBJ with physics cook data.
        std::string concaveMeshAssetName;
    };
}