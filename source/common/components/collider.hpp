//
// Created by mohse on 4/18/2026.
//

#pragma once
#include <glm/glm.hpp>
#include <reactphysics3d/reactphysics3d.h>
#include <algorithm>
#include <cctype>
#include <string>

#include "ecs/component.hpp"
#include "physics/physics-types.hpp"

namespace reactphysics3d {
    class TriangleMesh;
    class ConcaveMeshShape;
}

namespace our {

    /// ColliderComponent: Defines collision shape and properties for a rigid body
    class ColliderComponent : public Component {
    public:
        ColliderShape shape = ColliderShape::Box;

        glm::vec3 halfExtents{0.5f};
        glm::vec3 centerOffset{0.0f};
        float radius = 0.5f;
        float height = 1.8f;

        bool isTrigger = false;
        float friction = 0.5f;
        float restitution = 0.0f;

        /// When shape == ConcaveMesh: AssetLoader<Mesh> registry key (same as Mesh Renderer "mesh" name).
        std::string concaveMeshAssetName;

        /// Owned ReactPhysics3D resources for concave mesh (freed after the rigid body is destroyed).
        reactphysics3d::TriangleMesh* concaveTriangleMeshOwner{nullptr};
        reactphysics3d::ConcaveMeshShape* concaveMeshShapeOwner{nullptr};

        rp3d::Collider* colliderHandle = nullptr;

        static std::string getID() { return "Collider"; }

        ColliderDesc getDesc() const {
            ColliderDesc desc;
            desc.shape = shape;
            desc.halfExtents = halfExtents;
            desc.centerOffset = centerOffset;
            desc.radius = radius;
            desc.height = height;
            desc.isTrigger = isTrigger;
            desc.friction = friction;
            desc.restitution = restitution;
            desc.concaveMeshAssetName = concaveMeshAssetName;
            return desc;
        }

        void deserialize(const nlohmann::json& data) override {
            if (data.contains("shape")) {
                std::string shapeStr = data["shape"];
                std::transform(shapeStr.begin(), shapeStr.end(), shapeStr.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (shapeStr == "box") shape = ColliderShape::Box;
                else if (shapeStr == "sphere") shape = ColliderShape::Sphere;
                else if (shapeStr == "capsule") shape = ColliderShape::Capsule;
                else if (shapeStr == "concavemesh" || shapeStr == "concave_mesh") shape = ColliderShape::ConcaveMesh;
            }
            if (data.contains("halfExtents")) {
                auto& ext = data["halfExtents"];
                halfExtents = glm::vec3(ext[0], ext[1], ext[2]);
            }
            if (data.contains("centerOffset")) {
                auto& o = data["centerOffset"];
                centerOffset = glm::vec3(o[0], o[1], o[2]);
            }
            if (data.contains("radius")) radius = data["radius"];
            if (data.contains("height")) height = data["height"];
            if (data.contains("isTrigger")) isTrigger = data["isTrigger"];
            if (data.contains("friction")) friction = data["friction"];
            if (data.contains("restitution")) restitution = data["restitution"];
            if (data.contains("mesh")) concaveMeshAssetName = data["mesh"].get<std::string>();
        }
    };

}
