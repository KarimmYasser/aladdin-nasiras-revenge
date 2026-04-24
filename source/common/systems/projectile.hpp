#pragma once

#include "../audio/audio-system.hpp"
#include "../components/breakable.hpp"
#include "../components/enemy.hpp"
#include "../components/projectile.hpp"
#include "../ecs/world.hpp"
#include "../physics/physics-system.hpp"
#include <iostream>


namespace our {

class ProjectileSystem {
public:
  void update(World *world, PhysicsSystem *physicsSystem, float deltaTime) {
    if (!world || !physicsSystem)
      return;

    auto &physicsWorld = physicsSystem->getPhysicsWorld();
    std::vector<Entity *> toRemove;

    struct LootRequest {
      glm::vec3 position;
      LootEntry entry;
    };
    std::vector<LootRequest> lootRequests;

    for (auto entity : world->getEntities()) {
      auto *projectile = entity->getComponent<ProjectileComponent>();
      if (!projectile)
        continue;

      // 1. Handle Lifetime
      projectile->lifetime -= deltaTime;
      if (projectile->lifetime <= 0.0f) {
        toRemove.push_back(entity);
        continue;
      }

      // 2. Check for collisions using physics trigger events
      bool hitSomething = false;
      for (auto other : world->getEntities()) {
        if (other == entity || other == projectile->owner)
          continue;

        if (physicsWorld.hasAnyInteraction(entity, other, true)) {
          // Hit an Enemy?
          if (auto *enemy = other->getComponent<EnemyComponent>()) {
            if (enemy->currentState != EnemyComponent::State::DEAD) {
              enemy->health -= (int)projectile->damage;
              std::cout << "[ProjectileSystem] Apple hit " << other->name << "! Damage: " << projectile->damage << std::endl;

              if (enemy->health <= 0) {
                enemy->currentState = EnemyComponent::State::DEAD;
                AudioSystem::instance().playSound("assets/audio/death.wav");
              } else {
                AudioSystem::instance().playSound("assets/audio/hit.wav");
              }
              hitSomething = true;
            }
          }
          // Hit a Breakable (Pot)?
          else if (auto *breakable = other->getComponent<BreakableComponent>()) {
            hitSomething = true;
            std::cout << "[ProjectileSystem] Broke " << other->name << "!" << std::endl;

            // Collect loot requests
            for (size_t i = 0; i < breakable->lootItems.size(); ++i) {
              float angle = ((float)i / (float)breakable->lootItems.size()) * 2.0f * glm::pi<float>();
              float radius = 3.5f; // Wider scatter to prevent instant pickup
              glm::vec3 scatterOffset = glm::vec3(glm::cos(angle) * radius, 1.2f, glm::sin(angle) * radius);
              lootRequests.push_back({other->localTransform.position + scatterOffset, breakable->lootItems[i]});
            }
            physicsWorld.destroyRigidBody(other);
            world->markForRemoval(other);
            AudioSystem::instance().playSound("assets/audio/potBreak.mp3");
          }
          // Hit a wall or floor?
          else {
            auto *rb = other->getComponent<RigidBodyComponent>();
            if (rb && rb->type == RigidBodyType::Static) {
              hitSomething = true;
            }
          }

          if (hitSomething && projectile->destroyedOnImpact) {
            toRemove.push_back(entity);
            break;
          }
        }
      }
    }

    // Process removals
    for (auto entity : toRemove) {
      world->markForRemoval(entity);
    }

    // Process loot requests after iteration is safe
    for (const auto &req : lootRequests) {
      Entity *loot = world->add();
      loot->name = "Dropped_" + req.entry.type;
      loot->localTransform.position = req.position;

      auto mr = loot->addComponent<MeshRendererComponent>();
      auto coll = loot->addComponent<CollectibleComponent>();
      coll->pickupDelay = 0.6f; // Delay pickup

      if (req.entry.type == "coin") {
        mr->mesh = AssetLoader<Mesh>::get("coin_mesh");
        mr->material = AssetLoader<Material>::get("coin-mat");
        loot->localTransform.scale = glm::vec3(4.6f);
        coll->type = CollectibleComponent::Type::COIN;
      } else if (req.entry.type == "apple") {
        mr->mesh = AssetLoader<Mesh>::get("apple_mesh");
        mr->material = AssetLoader<Material>::get("lit-apple");
        loot->localTransform.scale = glm::vec3(1.3f);
        coll->type = CollectibleComponent::Type::APPLE;
      } else {
        // Default/Gem case
        mr->mesh = AssetLoader<Mesh>::get("cube");
        mr->material = AssetLoader<Material>::get("coin-mat");
        loot->localTransform.scale = glm::vec3(0.5f);
        coll->type = CollectibleComponent::Type::GEM;
      }
      coll->value = req.entry.value;

      auto rb = loot->addComponent<RigidBodyComponent>();
      rb->type = RigidBodyType::Static;
      auto lootCollider = loot->addComponent<ColliderComponent>();
      lootCollider->shape = ColliderShape::Sphere;
      lootCollider->radius = 0.5f;
      lootCollider->isTrigger = true;
    }
  }
};
} // namespace our
