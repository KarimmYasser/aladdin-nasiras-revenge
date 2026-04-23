#pragma once

#include "../ecs/component.hpp"

#include <glm/glm.hpp>
#include <string>

namespace our {

    enum class AladdinCameraMode : int {
        ThirdPerson = 0,
        FirstPerson = 1,
    };

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
        bool enableCameraFollow = false;
        float cameraSmoothing = 8.0f;
        AladdinCameraMode cameraMode = AladdinCameraMode::ThirdPerson;
        /// First-person: eye offset in character space (x = right, y = up, z = forward along view).
        glm::vec3 firstPersonCameraOffset = {0.0f, 1.38f, 0.22f};
        float firstPersonPitch = 0.0f;
        /// Logical yaw for character mesh facing (independent from camera).
        float facingYaw = 0.0f;
        /// Mesh is authored 180° from logical forward; added to `rotation.y` for rendering.
        static constexpr float meshYawVisualOffset = 3.14159265f;

        // Mouse-controlled orbit camera (like Roblox / UE third-person)
        /// Camera orbit yaw — controlled by mouse X movement.
        float cameraOrbitYaw = 0.0f;
        /// Camera orbit pitch — controlled by mouse Y movement (clamped).
        float cameraOrbitPitch = -0.25f;
        /// Mouse look sensitivity.
        float mouseSensitivity = 0.003f;

        // Spring-arm camera (like UE Camera Boom)
        /// Maximum arm length (distance from focus point to camera).
        float cameraArmLength = 8.0f;
        /// Minimum distance the camera can be to the focus point (when pushed in by a wall).
        float cameraArmMinDist = 1.0f;
        /// Small offset to prevent camera from sitting exactly on a wall surface.
        float cameraWallOffset = 0.3f;
        /// How fast the camera arm recovers after being pushed in by a wall.
        float cameraArmRecoverSpeed = 5.0f;
        /// Current (smoothed) arm distance — may be shorter than armLength due to walls.
        float currentArmDist = 8.0f;
        /// Height of the focus/look-at target above the player's feet.
        float cameraFocusHeight = 1.5f;

        glm::vec3 velocity = {0, 0, 0};
        bool isGrounded = false;

        bool isAttacking = false;
        float attackTimer = 0.0f;
        std::vector<Entity*> hitEntities;
        bool isThrowing = false;
        float throwTimer = 0.0f;

        int coinCount = 0;
        int gemCount = 0;
        int appleCount = 10;
        bool hasKey = false;
        int health = 100;
        int lives = 3;
        glm::vec3 respawnPosition = {0, 0, 0};

        // Invincibility after taking damage
        float invincibilityTimer = 0.0f;  // Seconds of invincibility remaining
        float invincibilityDuration = 2.0f; // Default duration after being hit

        static std::string getID() { return "Aladdin Controller"; }

        /**
         * @brief Reads movement and jump properties from a JSON object.
         * 
         * @param data The JSON object containing "speed", "jumpForce", and "rotationSpeed".
         */
        void deserialize(const nlohmann::json& data) override {
            if (!data.is_object()) return;
            speed = data.value("speed", speed);
            jumpForce = data.value("jumpForce", jumpForce);
            rotationSpeed = data.value("rotationSpeed", rotationSpeed);

            health = data.value("health", health);
            lives = data.value("lives", lives);
            if (data.contains("respawnPosition")) {
                auto& v = data["respawnPosition"];
                if (v.is_array() && v.size() >= 3) {
                    respawnPosition = {v[0].get<float>(), v[1].get<float>(), v[2].get<float>()};
                }
            }
            invincibilityDuration = data.value("invincibilityDuration", invincibilityDuration);

            enableCameraFollow = data.value("enableCameraFollow", enableCameraFollow);
            cameraSmoothing = data.value("cameraSmoothing", cameraSmoothing);

            if (data.contains("firstPersonCameraOffset")) {
                auto& v = data["firstPersonCameraOffset"];
                if (v.is_array() && v.size() >= 3) {
                    firstPersonCameraOffset = {v[0].get<float>(), v[1].get<float>(), v[2].get<float>()};
                }
            }
            firstPersonPitch = glm::radians(data.value("firstPersonPitchDegrees", glm::degrees(firstPersonPitch)));
            if (data.contains("cameraMode")) {
                const std::string mode = data.value("cameraMode", std::string("third"));
                if (mode == "first" || mode == "firstPerson" || mode == "first_person") {
                    cameraMode = AladdinCameraMode::FirstPerson;
                } else {
                    cameraMode = AladdinCameraMode::ThirdPerson;
                }
            }

            if (data.contains("firstPersonEyeHeight")) {
                firstPersonCameraOffset.y = data.value("firstPersonEyeHeight", firstPersonCameraOffset.y);
            }
            if (data.contains("firstPersonPitch")) {
                firstPersonPitch = data.value("firstPersonPitch", firstPersonPitch);
            }

            // Orbit camera fields
            mouseSensitivity = data.value("mouseSensitivity", mouseSensitivity);
            cameraOrbitPitch = glm::radians(data.value("cameraOrbitPitchDegrees", glm::degrees(cameraOrbitPitch)));

            // Spring-arm camera fields
            cameraArmLength = data.value("cameraArmLength", cameraArmLength);
            cameraArmMinDist = data.value("cameraArmMinDist", cameraArmMinDist);
            cameraWallOffset = data.value("cameraWallOffset", cameraWallOffset);
            cameraArmRecoverSpeed = data.value("cameraArmRecoverSpeed", cameraArmRecoverSpeed);
            cameraFocusHeight = data.value("cameraFocusHeight", cameraFocusHeight);
            currentArmDist = cameraArmLength; // init to full length

            if (Entity* e = getOwner()) {
                facingYaw = e->localTransform.rotation.y - meshYawVisualOffset;
                while (facingYaw > glm::pi<float>()) facingYaw -= 2.0f * glm::pi<float>();
                while (facingYaw < -glm::pi<float>()) facingYaw += 2.0f * glm::pi<float>();
            }
        }
    };

}