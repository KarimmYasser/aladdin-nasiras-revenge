#include <physics/physics-world.hpp>

#include "components/rigid-body.hpp"
#include "components/collider.hpp"
#include "glm/detail/type_quat.hpp"
#include "logger.hpp"

namespace our {
    // HELPERS - Define early so they can be used throughout
    inline reactphysics3d::Vector3 toRP3D(const glm::vec3& vec3) {
        return reactphysics3d::Vector3(vec3.x, vec3.y, vec3.z);
    }

    inline glm::vec3 toGLM(const reactphysics3d::Vector3& vec3) {
        return glm::vec3(vec3.x, vec3.y, vec3.z);
    }

    inline reactphysics3d::Quaternion toRP3D(const glm::quat& q) {
        return reactphysics3d::Quaternion(q.x, q.y, q.z, q.w);
    }

    bool PhysicsWorld::initialize() {
        if (initialized) {
            Logger::info("PhysicsWorld", "Already initialized.");
            return true;
        }

        Logger::info("PhysicsWorld", "Initializing physics world...");
        physicsWorld = physicsCommon.createPhysicsWorld();

        if (!physicsWorld) {
            Logger::error("PhysicsWorld", "Failed to create physics world.");
            return false;
        }

        physicsWorld->setGravity(reactphysics3d::Vector3(0.0f, -9.81f, 0.0f));
        Logger::info("PhysicsWorld", "Initialization successful. Gravity set to (0, -9.81, 0).");
        return initialized = true;
    }


    void PhysicsWorld::shutdown() {
        if (!initialized) {
            Logger::info("PhysicsWorld", "Already shut down.");
            return;
        }

        Logger::info("PhysicsWorld", "Shutting down physics world...");
        if (physicsWorld) {
            physicsCommon.destroyPhysicsWorld(physicsWorld);
            physicsWorld = nullptr;
        }
        initialized = false;
        Logger::info("PhysicsWorld", "Shutdown complete.");
    }

    void PhysicsWorld::createRigidBody(Entity* entity, const RigidBodyDesc& desc) {
        if (!physicsWorld) {
            Logger::error("PhysicsWorld", "Physics world is not initialized when creating rigid body.");
            return;
        }
        if (!entity) {
            Logger::error("PhysicsWorld", "Cannot create rigid body for null entity.");
            return;
        }

        const auto* transform = &entity->localTransform;

        Logger::info("PhysicsWorld", "Creating rigid body for entity '", entity->name, "'...");

        // Initialize the rigid body transform from the entity's transform
        reactphysics3d::Transform rp3dTransform = reactphysics3d::Transform::identity();

        rp3dTransform.setPosition(reactphysics3d::Vector3(
            transform->position.x, transform->position.y, transform->position.z
        ));

        rp3dTransform.setOrientation(Transform::fromEulerAnglesToRP3DQuaternion(transform->rotation));

        auto body = physicsWorld->createRigidBody(rp3dTransform);
        if (!body) {
            Logger::error("PhysicsWorld", "Failed to create rigid body for entity '", entity->name, "'.");
            return;
        }

        switch (desc.type) {
            case RigidBodyType::Static:    body->setType(reactphysics3d::BodyType::STATIC); break;
            case RigidBodyType::Dynamic:   body->setType(reactphysics3d::BodyType::DYNAMIC); break;
            case RigidBodyType::Kinematic: body->setType(reactphysics3d::BodyType::KINEMATIC); break;
        }
        body->setMass(desc.mass);

        // Apply gravity setting (only for dynamic bodies)
        if (desc.type == RigidBodyType::Dynamic) {
            body->enableGravity(desc.useGravity);
            if (!desc.useGravity) {
                Logger::debug("PhysicsWorld", "Gravity disabled for entity '", entity->name, "'");
            }
        }

        // Lock rotation if needed - disable all rotation axes
        if (desc.lockRotation) {
            body->setAngularLockAxisFactor(reactphysics3d::Vector3(0, 0, 0));
            Logger::debug("PhysicsWorld", "Rotation locked for entity '", entity->name, "'");
        }

        // Apply initial velocity if provided
        if (glm::length(desc.initialVelocity) > 0.0f) {
            body->setLinearVelocity(reactphysics3d::Vector3(
                desc.initialVelocity.x,
                desc.initialVelocity.y,
                desc.initialVelocity.z
            ));
            Logger::debug("PhysicsWorld", "Applied initial velocity to entity '", entity->name,
                         "': (", desc.initialVelocity.x, ", ", desc.initialVelocity.y, ", ", desc.initialVelocity.z, ")");
        }

        // Store the Entity pointer as user data so we can retrieve it during raycasts
        body->setUserData(entity);

        // Store the physics body handle in the ECS component
        auto* rbComp = entity->getComponent<RigidBodyComponent>();
        if (!rbComp) {
            rbComp = entity->addComponent<RigidBodyComponent>();
        }
        if (rbComp) {
            rbComp->bodyHandle = body;
            Logger::info("PhysicsWorld", "Rigid body created successfully for entity '", entity->name, "' (mass: ",
                         desc.mass, ").");
        }
        else {
            Logger::error("PhysicsWorld", "Failed to add RigidBodyComponent to entity '", entity->name, "'.");
        }
    }


    void PhysicsWorld::createCollider(Entity* entity, const ColliderDesc& desc) {
        if (!physicsWorld) {
            Logger::error("PhysicsWorld", "Physics world is not initialized when creating collider.");
            return;
        }
        if (!entity) {
            Logger::error("PhysicsWorld", "Cannot create collider for null entity.");
            return;
        }

        auto* rbComp = entity->getComponent<RigidBodyComponent>();
        if (!rbComp || !rbComp->bodyHandle) {
            Logger::error("PhysicsWorld", "Entity '", entity->name,
                          "' does not have a valid RigidBodyComponent required for collider creation.");
            return;
        }

        auto* colliderComp = entity->getComponent<ColliderComponent>();
        if (!colliderComp) {
            colliderComp = entity->addComponent<ColliderComponent>();
        }

        reactphysics3d::CollisionShape* shape = nullptr;
        switch (desc.shape) {
        case ColliderShape::Box:
            shape = physicsCommon.createBoxShape(
                reactphysics3d::Vector3(desc.halfExtents.x, desc.halfExtents.y, desc.halfExtents.z)
            );
            Logger::debug("PhysicsWorld", "Created box collider for entity '", entity->name,
                         "' with half-extents (", desc.halfExtents.x, ", ", desc.halfExtents.y, ", ", desc.halfExtents.z, ")");
            break;
        case ColliderShape::Sphere:
            shape = physicsCommon.createSphereShape(desc.radius);
            Logger::debug("PhysicsWorld", "Created sphere collider for entity '", entity->name,
                         "' with radius ", desc.radius);
            break;
        case ColliderShape::Capsule:
            shape = physicsCommon.createCapsuleShape(
                desc.radius,
                desc.height
            );
            Logger::debug("PhysicsWorld", "Created capsule collider for entity '", entity->name,
                         "' with radius ", desc.radius, " and height ", desc.height);
            break;
        default:
            Logger::error("PhysicsWorld", "Unknown collider shape: ", static_cast<int>(desc.shape));
            return;
        }

        if (!shape) {
            Logger::error("PhysicsWorld", "Failed to create collision shape for entity '", entity->name, "'.");
            return;
        }

        // Create the collider and store the handle
        auto* collider = rbComp->bodyHandle->addCollider(shape, reactphysics3d::Transform::identity());
        if (!collider) {
            Logger::error("PhysicsWorld", "Failed to add collider to rigid body for entity '", entity->name, "'.");
            return;
        }

        // Apply material properties (friction and restitution)
        reactphysics3d::Material& material = collider->getMaterial();
        material.setFrictionCoefficient(desc.friction);
        material.setBounciness(desc.restitution);
        Logger::debug("PhysicsWorld", "Applied material properties - Friction: ", desc.friction,
                     ", Restitution: ", desc.restitution);

        // Set as trigger if needed (trigger colliders don't produce contact forces)
        if (desc.isTrigger) {
            collider->setIsTrigger(true);
            Logger::debug("PhysicsWorld", "Collider for entity '", entity->name, "' set as trigger");
        }

        // Store the collider handle in the component
        if (colliderComp) {
            colliderComp->colliderHandle = collider;
            Logger::info("PhysicsWorld", "Collider created successfully for entity '", entity->name, "'.");
        }
    }

    void PhysicsWorld::step(float dt) const {
        if (!initialized) {
            Logger::error("PhysicsWorld", "Cannot step physics world - it is not initialized.");
            return;
        }
        physicsWorld->update(dt);
    }

    void PhysicsWorld::setLinearVelocity(Entity* entity, const glm::vec3& velocity) const {
        if (!physicsWorld) {
            Logger::error("PhysicsWorld", "Physics world is not initialized when setting linear velocity.");
            return;
        }
        if (!entity) {
            Logger::error("PhysicsWorld", "Cannot set linear velocity for null entity.");
            return;
        }

        auto* rbComp = entity->getComponent<RigidBodyComponent>();
        if (!rbComp || !rbComp->bodyHandle) {
            Logger::error("PhysicsWorld", "Entity '", entity->name,
                          "' does not have a valid RigidBodyComponent required for setting linear velocity.");
            return;
        }

        rbComp->bodyHandle->setLinearVelocity(reactphysics3d::Vector3(
            velocity.x, velocity.y, velocity.z
        ));
    }


    glm::vec3 PhysicsWorld::getLinearVelocity(Entity* entity) const {
        if (!physicsWorld) {
            Logger::error("PhysicsWorld", "Physics world is not initialized when getting linear velocity.");
            return glm::vec3();
        }
        if (!entity) {
            Logger::error("PhysicsWorld", "Cannot get linear velocity for null entity.");
            return glm::vec3();
        }
        auto* rbComp = entity->getComponent<RigidBodyComponent>();
        if (!rbComp || !rbComp->bodyHandle) {
            Logger::error("PhysicsWorld", "Entity '", entity->name,
                          "' does not have a valid RigidBodyComponent required for getting linear velocity.");
            return glm::vec3();
        }

        return toGLM(rbComp->bodyHandle->getLinearVelocity());
    }

    class ClosestHitCallback : public reactphysics3d::RaycastCallback {
    public:
        RaycastHit res;

        virtual reactphysics3d::decimal notifyRaycastHit(
            const reactphysics3d::RaycastInfo& info
        ) override {
            res.hasHit = true;
            res.point = toGLM(info.worldPoint);
            res.normal = toGLM(info.worldNormal);
            res.distance = info.hitFraction;

            // Retrieve and store the entity that was hit
            if (!info.body->getUserData()) {
                Logger::warning("PhysicsWorld", "Raycast hit a body with no user data!");
                res.entity = nullptr;
            } else {
                res.entity = static_cast<Entity*>(info.body->getUserData());
            }

            return info.hitFraction; // Return the hit fraction to find the closest hit
        }
    };

    RaycastHit PhysicsWorld::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
        if (!physicsWorld) {
            Logger::error("PhysicsWorld", "Physics world is not initialized when performing raycast.");
            return RaycastHit();
        }
        const glm::vec3 endPoint = origin + direction * maxDistance;

        reactphysics3d::Ray ray(toRP3D(origin), toRP3D(endPoint));
        ClosestHitCallback closestHit;

        physicsWorld->raycast(ray, &closestHit);
        if (closestHit.res.hasHit) {
            Logger::info("PhysicsWorld", "Raycast hit at point (", closestHit.res.point.x, ", ",
                         closestHit.res.point.y, ", ", closestHit.res.point.z, ") with normal (",
                         closestHit.res.normal.x, ", ", closestHit.res.normal.y, ", ", closestHit.res.normal.z,
                         ") at distance ", closestHit.res.distance);
            closestHit.res.distance *= maxDistance; // Scale the distance by maxDistance to get the actual distance
        } else {
            Logger::info("PhysicsWorld", "Raycast did not hit any object.");
        }
        return closestHit.res;
    }

}
