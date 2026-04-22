#pragma once

#include "../ecs/component.hpp"

#include <glm/glm.hpp>

namespace our {

    /**
     * @brief This component defines the properties and state for Aladdin.
     * 
     * It holds data for movement speed, jumping mechanics, and player state (like coin count).
     * This data is used by the AladdinControllerSystem to move the character and handle logic.
     */
    class AladdinControllerComponent : public Component {
    public:
        // Movement configuration
        float speed = 5.0f;               // Horizontal movement speed
        float jumpForce = 8.0f;           // Initial upward force when jumping
        float rotationSpeed = 10.0f;      // How fast the character turns towards movement direction
        
        // Camera Follow configuration
        bool enableCameraFollow = false;  // Whether the camera should follow Aladdin
        glm::vec3 cameraOffset = {0.0f, 10.0f, 15.0f}; // Default offset (above and behind)
        float cameraSmoothing = 5.0f;     // How smooth the camera follows (0 = instant, higher = smoother/slower)

        // Current runtime state (not usually serialized)
        // TODO (Physics): Once the PhysicsSystem is implemented, these manual velocity
        // and grounded state flags should be replaced or synchronized with the Rigidbody/Collider state.
        glm::vec3 velocity = {0, 0, 0};   // Current 3D velocity vector
        bool isGrounded = false;          // Whether the player is on the floor
        
        // TODO (Graphics/Animation): Once Member 1 finishes the Animation system, 
        // replace these booleans with triggers for the AnimatorComponent.
        bool isAttacking = false;         // Whether Aladdin is performing a sword attack
        float attackTimer = 0.0f;         // Timer for sword attack duration
        std::vector<Entity*> hitEntities; // List of entities already hit in the current attack
        bool isThrowing = false;          // Whether Aladdin is throwing an apple
        float throwTimer = 0.0f;          // Timer for throw animation duration

        // Collectibles
        int coinCount = 0;                // Number of collected ancient coins
        int gemCount = 0;                 // Number of collected gems
        int appleCount = 10;              // Number of apples available for throwing
        bool hasKey = false;              // Does the player have the key to exit?
        int health = 100;                 // Current health (hearts/points)
        int lives = 3;                    // Current lives (retry attempts)
        glm::vec3 respawnPosition = {0, 0, 0}; // Position to return to on death

        // Invincibility after taking damage
        float invincibilityTimer = 0.0f;  // Seconds of invincibility remaining
        float invincibilityDuration = 2.0f; // Default duration after being hit

        // The ID of this component type is "Aladdin Controller"
        static std::string getID() { return "Aladdin Controller"; }

        /**
         * @brief Reads movement and jump properties from a JSON object.
         * 
         * @param data The JSON object containing "speed", "jumpForce", and "rotationSpeed".
         */
        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            speed = data.value("speed", speed);
            jumpForce = data.value("jumpForce", jumpForce);
            rotationSpeed = data.value("rotationSpeed", rotationSpeed);

            health = data.value("health", health);
            lives = data.value("lives", lives);
            if(data.contains("respawnPosition")){
                auto& v = data["respawnPosition"];
                if(v.is_array() && v.size() >= 3){
                    respawnPosition = {v[0].get<float>(), v[1].get<float>(), v[2].get<float>()};
                }
            }
            invincibilityDuration = data.value("invincibilityDuration", invincibilityDuration);

            enableCameraFollow = data.value("enableCameraFollow", enableCameraFollow);
            if(data.contains("cameraOffset")){
                auto& v = data["cameraOffset"];
                if(v.is_array() && v.size() >= 3){
                    cameraOffset = {v[0].get<float>(), v[1].get<float>(), v[2].get<float>()};
                }
            }
            cameraSmoothing = data.value("cameraSmoothing", cameraSmoothing);
        }
    };

}