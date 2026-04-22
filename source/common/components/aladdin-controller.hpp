#pragma once

#include "../ecs/component.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <GLFW/glfw3.h>
#include <string>

namespace our {

    enum class AladdinCameraMode : int {
        ThirdPerson = 0, // behind the player
        FirstPerson = 1, // in the player's perspective (POV)
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
        bool enableCameraFollow = false;  // Whether the camera should follow Aladdin
        /// Third-person offset: x = shoulder (+right), y = height above feet, z = distance behind (positive = behind).
        glm::vec3 cameraOffset = {0.0f, 10.0f, 15.0f};
        float cameraSmoothing = 5.0f;     // How smooth the camera follows (0 = instant, higher = smoother/slower)
        AladdinCameraMode cameraMode = AladdinCameraMode::ThirdPerson;
        /// First-person: eye height (y) and small forward (z) along view so the lens sits in front of the head, not inside the mesh.
        glm::vec3 firstPersonCameraOffset = {0.0f, 1.38f, 0.22f};
        float firstPersonPitch = 0.0f;    // Extra look pitch (radians), applied in first person only
        /// Third-person: slight downward pitch (radians). Yaw comes from facingYaw (movement), not physics euler.
        float thirdPersonPitch = -0.14f;
        /// Extra yaw/pitch from mouse look (radians). View yaw = facingYaw + cameraYawOffset.
        float cameraYawOffset = 0.0f;
        float cameraPitchOffset = 0.0f;
        /// Radians per pixel while holding `mouseLookButton` (GLFW constant, default RMB).
        float mouseLookSensitivity = 0.0025f;
        /// GLFW mouse button for orbit look (default: right button). Hold + move to look; cursor locks while held.
        int mouseLookButton = GLFW_MOUSE_BUTTON_RIGHT;
        /// Horizontal facing used by movement and cameras (logical yaw; independent of physics euler).
        float facingYaw = 0.0f;
        /// Aladdin mesh is authored 180° from this logical forward; add to `rotation.y` for rendering only.
        static constexpr float meshYawVisualOffset = 3.14159265f;

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
        int enemyCount = 0;               // Number of defeated enemies
        bool hasKey = false;              // Does the player have the key to exit?
        int health = 100;                 // Current health (hearts/points)
        int lives = 3;                    // Current lives (retry attempts)
        glm::vec3 respawnPosition = {0, 0, 0}; // Position to return to on death

        // Invincibility after taking damage
        float invincibilityTimer = 0.0f;  // Seconds of invincibility remaining
        float invincibilityDuration = 2.0f; // Default duration after being hit

        /// Runtime: cursor locked for follow-cam mouse look (unlocked on state exit / camera mode change).
        bool followCamMouseLocked = false;
        /// Skip one delta after locking to avoid a spike when the OS warps the cursor to center.
        bool followCamSkipNextLookDelta = false;
        /// Last cursor position for follow-cam look (GLFW); used when ImGui disables our `Mouse` helper.
        double lastCamLookCx = 0.0;
        double lastCamLookCy = 0.0;

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

            if(data.contains("firstPersonCameraOffset")){
                auto& v = data["firstPersonCameraOffset"];
                if(v.is_array() && v.size() >= 3){
                    firstPersonCameraOffset = {v[0].get<float>(), v[1].get<float>(), v[2].get<float>()};
                }
            }
            firstPersonPitch = glm::radians(data.value("firstPersonPitchDegrees", 0.0f));
            thirdPersonPitch = glm::radians(data.value("thirdPersonPitchDegrees", glm::degrees(thirdPersonPitch)));
            mouseLookSensitivity = data.value("mouseLookSensitivity", mouseLookSensitivity);
            if(data.contains("mouseLookButton")) {
                const std::string b = data["mouseLookButton"].get<std::string>();
                if(b == "left" || b == "LMB") mouseLookButton = GLFW_MOUSE_BUTTON_LEFT;
                else if(b == "middle" || b == "MMB") mouseLookButton = GLFW_MOUSE_BUTTON_MIDDLE;
                else mouseLookButton = GLFW_MOUSE_BUTTON_RIGHT;
            }
            if(data.contains("cameraMode")){
                const std::string mode = data.value("cameraMode", std::string("third"));
                if(mode == "first" || mode == "firstPerson" || mode == "first_person"){
                    cameraMode = AladdinCameraMode::FirstPerson;
                } else {
                    cameraMode = AladdinCameraMode::ThirdPerson;
                }
            }

            if(Entity* e = getOwner()) {
                facingYaw = e->localTransform.rotation.y - meshYawVisualOffset;
                while(facingYaw > glm::pi<float>()) facingYaw -= 2.0f * glm::pi<float>();
                while(facingYaw < -glm::pi<float>()) facingYaw += 2.0f * glm::pi<float>();
            }
        }
    };

}
