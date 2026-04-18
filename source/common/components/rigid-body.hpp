//
// Created by mohse on 4/18/2026.
//


#pragma once
#include <glm/glm.hpp>
#include <reactphysics3d/reactphysics3d.h>
#include <algorithm>
#include <cctype>

#include "ecs/component.hpp"
#include "physics/physics-types.hpp"

namespace our {
    /// RigidBodyComponent: Encapsulates rigid body physics properties
    ///
    /// This component represents a rigid body in the physics simulation. It stores
    /// physical properties (type, mass, gravity, etc.) and maintains a handle to the
    /// underlying ReactPhysics3D rigid body for integration with the physics engine.
    ///
    /// The bodyHandle is managed by the PhysicsWorld and should never be deleted manually.
    /// When this component is removed, the associated rigid body must be destroyed through
    /// the physics system.
    class RigidBodyComponent : public Component {
    public:
        RigidBodyType type = RigidBodyType::Dynamic;
        float mass = 1.0f;
        bool useGravity = true;
        bool lockRotation = true;
        glm::vec3 velocity{0.0f};

        // ---- ReactPhysics3D Integration ----

        /// Pointer to the ReactPhysics3D rigid body
        ///
        /// This handle is created by PhysicsWorld::createRigidBody(const Transform&)
        /// and is managed by the physics world. The physics world is responsible for
        /// destroying this pointer when the component is destroyed.
        ///
        /// Usage:
        ///   if (bodyHandle != nullptr) {
        ///       rp3d::Vector3 rbVelocity = bodyHandle->getLinearVelocity();
        ///       bodyHandle->setLinearVelocity(rbVelocity);
        ///   }
        rp3d::RigidBody* bodyHandle{nullptr};

        static std::string getID() {
            return "RigidBody";
        }

        RigidBodyDesc getDesc() const {
            RigidBodyDesc desc;
            desc.type = type;
            desc.mass = mass;
            desc.useGravity = useGravity;
            desc.lockRotation = lockRotation;
            desc.initialVelocity = velocity;
            return desc;
        }

        void deserialize(const nlohmann::json& data) override {
            if (data.contains("bodyType")) {
                if (data["bodyType"].is_number_integer()) {
                    const int rawType = data["bodyType"].get<int>();
                    switch (rawType) {
                    case 0: type = RigidBodyType::Static; break;
                    case 1: type = RigidBodyType::Dynamic; break;
                    case 2: type = RigidBodyType::Kinematic; break;
                    default: type = RigidBodyType::Dynamic; break;
                    }
                } else if (data["bodyType"].is_string()) {
                    std::string typeString = data["bodyType"].get<std::string>();
                    std::transform(typeString.begin(), typeString.end(), typeString.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    if (typeString == "static") type = RigidBodyType::Static;
                    else if (typeString == "dynamic") type = RigidBodyType::Dynamic;
                    else if (typeString == "kinematic") type = RigidBodyType::Kinematic;
                    else type = RigidBodyType::Dynamic;
                }
            }
            if (data.contains("mass")) mass = data["mass"];
            if (data.contains("useGravity")) useGravity = data["useGravity"];
            if (data.contains("lockRotation")) lockRotation = data["lockRotation"];
            if (data.contains("velocity")) {
                auto& vel = data["velocity"];
                velocity = glm::vec3(vel[0], vel[1], vel[2]);
            }
        }
    };
}
