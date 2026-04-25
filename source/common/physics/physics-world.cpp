#include <physics/physics-world.hpp>

#include <cstdint>

#include "asset-loader.hpp"
#include "components/rigid-body.hpp"
#include "components/collider.hpp"
#include "glm/detail/type_quat.hpp"
#include "logger.hpp"
#include "mesh/mesh.hpp"
#include "reactphysics3d/collision/OverlapCallback.h"
#include "reactphysics3d/collision/TriangleVertexArray.h"
#include "reactphysics3d/engine/EventListener.h"
#include "reactphysics3d/utils/Message.h"

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

    static PhysicsEventPhase toContactPhase(const reactphysics3d::CollisionCallback::ContactPair::EventType eventType) {
        switch (eventType) {
            case reactphysics3d::CollisionCallback::ContactPair::EventType::ContactStart: return PhysicsEventPhase::Begin;
            case reactphysics3d::CollisionCallback::ContactPair::EventType::ContactStay: return PhysicsEventPhase::Stay;
            case reactphysics3d::CollisionCallback::ContactPair::EventType::ContactExit: return PhysicsEventPhase::End;
            default: return PhysicsEventPhase::Stay;
        }
    }

    static PhysicsEventPhase toTriggerPhase(const reactphysics3d::OverlapCallback::OverlapPair::EventType eventType) {
        switch (eventType) {
            case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapStart: return PhysicsEventPhase::Begin;
            case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapStay: return PhysicsEventPhase::Stay;
            case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapExit: return PhysicsEventPhase::End;
            default: return PhysicsEventPhase::Stay;
        }
    }

    class PhysicsWorldEventListener final : public reactphysics3d::EventListener {
    private:
        PhysicsWorld* owner;

    public:
        explicit PhysicsWorldEventListener(PhysicsWorld* owner): owner(owner) {}

        void onContact(const reactphysics3d::CollisionCallback::CallbackData& callbackData) override {
            if (!owner) return;

            const auto pairCount = callbackData.getNbContactPairs();
            for (reactphysics3d::uint32 i = 0; i < pairCount; i++) {
                const auto pair = callbackData.getContactPair(i);
                auto* bodyA = pair.getBody1();
                auto* bodyB = pair.getBody2();
                if (!bodyA || !bodyB) continue;

                auto* entityA = static_cast<Entity*>(bodyA->getUserData());
                auto* entityB = static_cast<Entity*>(bodyB->getUserData());
                if (!entityA || !entityB) continue;

                PhysicsEvent event;
                event.type = PhysicsEventType::Contact;
                event.phase = toContactPhase(pair.getEventType());
                event.entityA = entityA;
                event.entityB = entityB;

                if (pair.getNbContactPoints() > 0) {
                    auto point = pair.getContactPoint(0);
                    const auto& normal = point.getWorldNormal();
                    event.normal = glm::vec3(normal.x, normal.y, normal.z);
                    event.penetrationDepth = static_cast<float>(point.getPenetrationDepth());
                }

                owner->contactEvents.push_back(event);
            }
        }

        void onTrigger(const reactphysics3d::OverlapCallback::CallbackData& callbackData) override {
            if (!owner) return;

            const auto pairCount = callbackData.getNbOverlappingPairs();
            for (reactphysics3d::uint32 i = 0; i < pairCount; i++) {
                const auto pair = callbackData.getOverlappingPair(i);
                auto* bodyA = pair.getBody1();
                auto* bodyB = pair.getBody2();
                if (!bodyA || !bodyB) continue;

                auto* entityA = static_cast<Entity*>(bodyA->getUserData());
                auto* entityB = static_cast<Entity*>(bodyB->getUserData());
                if (!entityA || !entityB) continue;

                PhysicsEvent event;
                event.type = PhysicsEventType::Trigger;
                event.phase = toTriggerPhase(pair.getEventType());
                event.entityA = entityA;
                event.entityB = entityB;

                owner->triggerEvents.push_back(event);
            }
        }
    };

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

        if (eventListener) {
            delete eventListener;
            eventListener = nullptr;
        }
        eventListener = new PhysicsWorldEventListener(this);
        physicsWorld->setEventListener(eventListener);

        clearFrameEvents();
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
            physicsWorld->setEventListener(nullptr);
            physicsCommon.destroyPhysicsWorld(physicsWorld);
            physicsWorld = nullptr;
        }

        if (eventListener) {
            delete eventListener;
            eventListener = nullptr;
        }

        clearFrameEvents();
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

        if (!rp3dTransform.isValid()) {
            Logger::warning("PhysicsWorld", "Invalid transform for entity '", entity->name,
                            "'. Resetting to identity transform before rigid body creation.");
            rp3dTransform = reactphysics3d::Transform::identity();
        }

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

    void PhysicsWorld::destroyRigidBody(Entity* entity) {
        if (!physicsWorld || !entity) return;
        auto* rbComp = entity->getComponent<RigidBodyComponent>();
        auto* colliderComp = entity->getComponent<ColliderComponent>();

        if (rbComp && rbComp->bodyHandle) {
            Logger::info("PhysicsWorld", "Destroying rigid body for entity '", entity->name, "'");
            physicsWorld->destroyRigidBody(rbComp->bodyHandle);
            rbComp->bodyHandle = nullptr;
        }

        if (colliderComp) {
            colliderComp->colliderHandle = nullptr;
            if (colliderComp->concaveMeshShapeOwner) {
                physicsCommon.destroyConcaveMeshShape(colliderComp->concaveMeshShapeOwner);
                colliderComp->concaveMeshShapeOwner = nullptr;
            }
            if (colliderComp->concaveTriangleMeshOwner) {
                physicsCommon.destroyTriangleMesh(colliderComp->concaveTriangleMeshOwner);
                colliderComp->concaveTriangleMeshOwner = nullptr;
            }
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

        if (desc.shape == ColliderShape::ConcaveMesh) {
            if (rbComp && rbComp->type != RigidBodyType::Static) {
                Logger::error("PhysicsWorld", "Concave mesh collider on entity '", entity->name,
                              "' requires a static RigidBody.");
                return;
            }

            Mesh* meshAsset = AssetLoader<Mesh>::get(desc.concaveMeshAssetName);
            if (!meshAsset || !meshAsset->hasPhysicsTriangleData()) {
                Logger::error("PhysicsWorld", "Concave mesh collider: missing mesh '", desc.concaveMeshAssetName,
                              "' or mesh has no physics cook data for entity '", entity->name, "'.");
                return;
            }

            const auto& posBuf = meshAsset->getPhysicsVertexPositions();
            const auto& idxBuf = meshAsset->getPhysicsIndices();
            reactphysics3d::TriangleVertexArray vertexArray(
                meshAsset->getPhysicsVertexCount(),
                posBuf.data(),
                static_cast<uint32_t>(sizeof(float) * 3),
                meshAsset->getPhysicsTriangleCount(),
                idxBuf.data(),
                static_cast<uint32_t>(sizeof(uint32_t) * 3),
                reactphysics3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                reactphysics3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE
            );

            std::vector<reactphysics3d::Message> messages;
            reactphysics3d::TriangleMesh* triMesh = physicsCommon.createTriangleMesh(vertexArray, messages);
            for (const auto& msg : messages) {
                Logger::warning("PhysicsWorld", "TriangleMesh '", desc.concaveMeshAssetName, "': ", msg.text);
            }
            if (!triMesh) {
                Logger::error("PhysicsWorld", "createTriangleMesh failed for entity '", entity->name, "'.");
                return;
            }

            const glm::vec3& sc = entity->localTransform.scale;
            reactphysics3d::Vector3 rpScale(sc.x, sc.y, sc.z);
            reactphysics3d::ConcaveMeshShape* concave =
                physicsCommon.createConcaveMeshShape(triMesh, rpScale);
            if (!concave) {
                Logger::error("PhysicsWorld", "createConcaveMeshShape failed for entity '", entity->name, "'.");
                physicsCommon.destroyTriangleMesh(triMesh);
                return;
            }

            colliderComp->concaveTriangleMeshOwner = triMesh;
            colliderComp->concaveMeshShapeOwner = concave;
            shape = concave;
            Logger::info("PhysicsWorld", "Created concave mesh collider for '", entity->name, "' from mesh '",
                         desc.concaveMeshAssetName, "'.");
        } else {
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
        }

        if (!shape) {
            Logger::error("PhysicsWorld", "Failed to create collision shape for entity '", entity->name, "'.");
            return;
        }

        // Collider pose in rigid-body local space (offset aligns box with offset mesh pivots)
        const reactphysics3d::Transform colliderLocal(
            toRP3D(desc.centerOffset),
            reactphysics3d::Quaternion::identity()
        );
        auto* collider = rbComp->bodyHandle->addCollider(shape, colliderLocal);
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
            closestHit.res.distance *= maxDistance; // Scale the distance by maxDistance to get the actual distance
        }
        return closestHit.res;
    }

    void PhysicsWorld::clearFrameEvents() {
        contactEvents.clear();
        triggerEvents.clear();
    }

    static bool matchesPair(const PhysicsEvent& event, const Entity* a, const Entity* b, const bool includeStay) {
        if (!event.entityA || !event.entityB || !a || !b) return false;
        if (!includeStay && event.phase == PhysicsEventPhase::Stay) return false;

        return (event.entityA == a && event.entityB == b) ||
               (event.entityA == b && event.entityB == a);
    }

    bool PhysicsWorld::hasContactEvent(Entity* a, Entity* b, const bool includeStay) const {
        if (!a || !b) return false;
        for (const auto& event : contactEvents) {
            if (matchesPair(event, a, b, includeStay) && event.phase != PhysicsEventPhase::End) {
                return true;
            }
        }
        return false;
    }

    bool PhysicsWorld::hasTriggerEvent(Entity* a, Entity* b, const bool includeStay) const {
        if (!a || !b) return false;
        for (const auto& event : triggerEvents) {
            if (matchesPair(event, a, b, includeStay) && event.phase != PhysicsEventPhase::End) {
                return true;
            }
        }
        return false;
    }

    bool PhysicsWorld::hasAnyInteraction(Entity* a, Entity* b, const bool includeStay) const {
        return hasContactEvent(a, b, includeStay) || hasTriggerEvent(a, b, includeStay) || testOverlap(a, b);
    }
    
    bool PhysicsWorld::testOverlap(Entity* a, Entity* b) const {
        if (!physicsWorld || !a || !b) return false;
        auto* rbA = a->getComponent<RigidBodyComponent>();
        auto* rbB = b->getComponent<RigidBodyComponent>();
        if (!rbA || !rbB || !rbA->bodyHandle || !rbB->bodyHandle) return false;

        return physicsWorld->testOverlap(rbA->bodyHandle, rbB->bodyHandle);
    }

    bool PhysicsWorld::isGrounded(Entity* entity, const float minUpDot) const {
        if (!entity) return false;

        for (const auto& event : contactEvents) {
            if (event.phase == PhysicsEventPhase::End) continue;
            if (event.entityA != entity && event.entityB != entity) continue;

            const float supportY = (event.entityA == entity) ? -event.normal.y : event.normal.y;
            if (supportY >= minUpDot) {
                return true;
            }
        }
        return false;
    }

}
