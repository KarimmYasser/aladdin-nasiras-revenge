#pragma once

#include "../ecs/world.hpp"
#include "../components/projectile.hpp"
#include "../components/enemy.hpp"
#include "../components/breakable.hpp"
#include "../physics/physics-system.hpp"
#include "../audio/audio-system.hpp"
#include <iostream>

namespace our {

    class ProjectileSystem {
    public:
        void update(World* world, PhysicsSystem* physicsSystem, float deltaTime) {
            if (!world || !physicsSystem) return;

            auto& physicsWorld = physicsSystem->getPhysicsWorld();
            std::vector<Entity*> toRemove;

            for (auto entity : world->getEntities()) {
                auto* projectile = entity->getComponent<ProjectileComponent>();
                if (!projectile) continue;

                // 1. Handle Lifetime
                projectile->lifetime -= deltaTime;
                if (projectile->lifetime <= 0.0f) {
                    toRemove.push_back(entity);
                    continue;
                }

                // 2. Check for collisions using physics trigger events
                // We check against all entities to see if our apple overlapped with them
                bool hitSomething = false;
                for (auto other : world->getEntities()) {
                    if (other == entity || other == projectile->owner) continue;

                    if (physicsWorld.hasAnyInteraction(entity, other, true)) {
                        
                        // Hit an Enemy?
                        if (auto* enemy = other->getComponent<EnemyComponent>()) {
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
                        else if (auto* breakable = other->getComponent<BreakableComponent>()) {
                            // We trigger the breakable logic by manually calling what the aladdin-controller does
                            // Or better: let the aladdin-controller handle breakables if we can, but projectiles are independent.
                            // For now, let's just log it. Real breakable logic usually involves spawning loot.
                            std::cout << "[ProjectileSystem] Apple smashed " << other->name << "!" << std::endl;
                            hitSomething = true;
                            // Note: To properly break it, we'd need to duplicate the loot spawning logic 
                            // or move it to a shared place. For this enhancement, hitting enemies is the priority.
                        }
                        // Hit a wall or floor?
                        else {
                            // If it's a static body (ground/wall), just destroy the apple
                            auto* rb = other->getComponent<RigidBodyComponent>();
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

            for (auto entity : toRemove) {
                world->markForRemoval(entity);
            }
        }
    };
}
