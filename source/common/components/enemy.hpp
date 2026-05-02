#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace our {

    /**
     * @brief Defines the state and behavior properties for an Enemy.
     */
    class EnemyComponent : public Component {
    public:
        enum class State {
            PATROL,
            CHASE,
            ATTACK,
            DEAD
        };

        // Configuration
        float patrolSpeed = 2.0f;
        float chaseSpeed = 3.5f;
        float detectionRange = 10.0f;
        float attackRange = 2.0f;
        int health = 50;
        int maxHealth = 50;
        int damage = 15;
        float attackCooldown = 1.5f;
        float idleAfterAttackDuration = 1.0f; // Seconds to stay idle after an attack

        // Patrol points
        std::vector<glm::vec3> waypoints;
        int currentWaypointIndex = 0;

        // Runtime State
        State currentState = State::PATROL;
        float currentAttackTimer = 0.0f;
        float idleTimer = 0.0f; // Timer for the idle state after attack
        float deathTimer = 0.0f; // For "corpse" duration before deletion
        float healthBarTimer = 0.0f; // Shows health bar for a duration after being hit
        float deathAnimationDuration = 3.0f; // Duration to wait for death animation to play before removal

        static std::string getID() { return "Enemy"; }

        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            patrolSpeed = data.value("patrolSpeed", patrolSpeed);
            chaseSpeed = data.value("chaseSpeed", chaseSpeed);
            detectionRange = data.value("detectionRange", detectionRange);
            attackRange = data.value("attackRange", attackRange);
            health = data.value("health", health);
            maxHealth = health; // Set initial max health to starting health
            damage = data.value("damage", damage);
            attackCooldown = data.value("attackCooldown", attackCooldown);

            if(data.contains("waypoints") && data["waypoints"].is_array()){
                waypoints.clear();
                for(const auto& wp : data["waypoints"]){
                    if(wp.is_array() && wp.size() >= 3){
                        waypoints.push_back({wp[0].get<float>(), wp[1].get<float>(), wp[2].get<float>()});
                    }
                }
            }
        }
    };

}
