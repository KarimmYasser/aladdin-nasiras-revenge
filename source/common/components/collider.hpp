//
// Created by mohse on 4/18/2026.
//


#pragma once
#include <glm/glm.hpp>
#include <reactphysics3d/reactphysics3d.h>

#include "ecs/component.hpp"
#include "physics/physics-types.hpp"

namespace our {

    /// ColliderComponent: Defines collision shape and properties for a rigid body
    ///
    /// This component stores the collision shape configuration (box, sphere, or capsule)
    /// and material properties (friction, restitution). It must be paired with a RigidBodyComponent
    /// to function in the physics world.
    class ColliderComponent : public Component {
    public:
        ColliderShape shape = ColliderShape::Box;

        glm::vec3 halfExtents{0.5f};
        float radius = 0.5f;
        float height = 1.8f;

        bool isTrigger = false;
        float friction = 0.5f;
        float restitution = 0.0f;

        /// Pointer to the ReactPhysics3D collider (managed by the physics world)
        ///
        /// Created by the physics system when the component is initialized.
        /// Use this to query collision information or modify collider properties at runtime.
        ///
        /// Usage:
        ///   if (colliderHandle != nullptr) {
        ///       rp3d::Material& material = colliderHandle->getMaterial();
        ///       material.setFrictionCoefficient(0.8f);
        ///   }
        rp3d::Collider* colliderHandle = nullptr;

        static std::string getID() { return "Collider"; }

        /**
         * @brief Get the descriptor for this collider component
         * @return ColliderDesc containing the current collider properties
         */
        ColliderDesc getDesc() const {
            ColliderDesc desc;
            desc.shape = shape;
            desc.halfExtents = halfExtents;
            desc.radius = radius;
            desc.height = height;
            desc.isTrigger = isTrigger;
            desc.friction = friction;
            desc.restitution = restitution;
            return desc;
        }

        /**
         * @brief Deserialize collider properties from JSON
         * @param data JSON object containing collider configuration
         */
        void deserialize(const nlohmann::json& data) override {
            if (data.contains("shape")) {
                std::string shapeStr = data["shape"];
                std::transform(shapeStr.begin(), shapeStr.end(), shapeStr.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (shapeStr == "box") shape = ColliderShape::Box;
                else if (shapeStr == "sphere") shape = ColliderShape::Sphere;
                else if (shapeStr == "capsule") shape = ColliderShape::Capsule;
            }
            if (data.contains("halfExtents")) {
                auto& ext = data["halfExtents"];
                halfExtents = glm::vec3(ext[0], ext[1], ext[2]);
            }
            if (data.contains("radius")) radius = data["radius"];
            if (data.contains("height")) height = data["height"];
            if (data.contains("isTrigger")) isTrigger = data["isTrigger"];
            if (data.contains("friction")) friction = data["friction"];
            if (data.contains("restitution")) restitution = data["restitution"];
        }
    };

}
