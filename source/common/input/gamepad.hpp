#pragma once

#include <GLFW/glfw3.h>
#include <cstring>
#include <cmath>

namespace our {

    // A convenience class to read gamepad (controller) input using GLFW's Gamepad API
    class Gamepad {
    private:
        bool enabled;
        bool connected;
        int joystickId;
        
        GLFWgamepadstate currentState;
        GLFWgamepadstate previousState;

    public:
        Gamepad(int jid = GLFW_JOYSTICK_1) : enabled(false), connected(false), joystickId(jid) {
            std::memset(&currentState, 0, sizeof(GLFWgamepadstate));
            std::memset(&previousState, 0, sizeof(GLFWgamepadstate));
        }

        void enable() {
            enabled = true;
            updateConnection();
            if (connected) {
                glfwGetGamepadState(joystickId, &currentState);
                previousState = currentState;
            }
        }

        void disable() {
            enabled = false;
            std::memset(&currentState, 0, sizeof(GLFWgamepadstate));
            std::memset(&previousState, 0, sizeof(GLFWgamepadstate));
        }

        void update() {
            if (!enabled) return;
            
            updateConnection();
            if (connected) {
                previousState = currentState;
                glfwGetGamepadState(joystickId, &currentState);
            } else {
                std::memset(&currentState, 0, sizeof(GLFWgamepadstate));
            }
        }

        void updateConnection() {
            connected = glfwJoystickIsGamepad(joystickId);
        }

        [[nodiscard]] bool isConnected() const { return connected; }
        
        // Button states
        [[nodiscard]] bool isPressed(int button) const {
            if (!connected || button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
            return currentState.buttons[button] == GLFW_PRESS;
        }

        [[nodiscard]] bool justPressed(int button) const {
            if (!connected || button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
            return currentState.buttons[button] == GLFW_PRESS && previousState.buttons[button] == GLFW_RELEASE;
        }

        [[nodiscard]] bool justReleased(int button) const {
            if (!connected || button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
            return currentState.buttons[button] == GLFW_RELEASE && previousState.buttons[button] == GLFW_PRESS;
        }

        // Axis states (Sticks and Triggers)
        // Note: GLFW maps stick axes from -1.0 to 1.0, and triggers from -1.0 to 1.0 (some platforms 0 to 1)
        [[nodiscard]] float getAxis(int axis) const {
            if (!connected || axis < 0 || axis > GLFW_GAMEPAD_AXIS_LAST) return 0.0f;
            return currentState.axes[axis];
        }

        // Helper with a small deadzone to prevent drift
        [[nodiscard]] float getAxisWithDeadzone(int axis, float deadzone = 0.15f) const {
            float val = getAxis(axis);
            if (std::abs(val) < deadzone) return 0.0f;
            // Rescale so that output goes smoothly from 0 after deadzone
            return (val - (val > 0 ? deadzone : -deadzone)) / (1.0f - deadzone);
        }

        [[nodiscard]] bool isEnabled() const { return enabled; }
        void setEnabled(bool enabled) {
            if(this->enabled != enabled) {
                if (enabled) enable();
                else disable();
            }
        }
    };

}
