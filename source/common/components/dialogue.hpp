#pragma once

#include "../ecs/component.hpp"
#include <json/json.hpp>

#include <string>
#include <vector>

namespace our {

    /**
     * @brief Attach to a "trigger" entity (typically a lamp) that starts a
     *        cutscene-style conversation when the player enters its radius.
     */
    class DialogueTriggerComponent : public Component {
    public:
        std::string scriptPath;
        std::string stylePath = "config/levels/dialogues/style.json";

        // Distance to trigger the meeting.
        float radius = 4.5f;
        // If true the trigger fires once and never again.
        bool oneShot = true;

        // Runtime flag — set by DialogueSystem after the meeting starts.
        bool fired = false;

        // Name of the entity to make visible at meeting start (e.g. the Genie).
        std::string revealEntity;
        // Entities to mark for removal when the conversation ends.
        std::vector<std::string> removeEntitiesOnFinish;
        // Entities to make visible when the conversation ends (e.g. the "used" lamp).
        std::vector<std::string> revealEntitiesOnFinish;

        static std::string getID() { return "Dialogue Trigger"; }

        void deserialize(const nlohmann::json& data) override {
            if (!data.is_object()) return;
            scriptPath        = data.value("script",        scriptPath);
            stylePath         = data.value("style",         stylePath);
            radius            = data.value("radius",        radius);
            oneShot           = data.value("oneShot",       oneShot);
            revealEntity      = data.value("revealEntity",  revealEntity);
            if (data.contains("removeEntitiesOnFinish") && data["removeEntitiesOnFinish"].is_array()) {
                for (const auto& v : data["removeEntitiesOnFinish"]) {
                    if (v.is_string()) removeEntitiesOnFinish.push_back(v.get<std::string>());
                }
            }
            if (data.contains("revealEntitiesOnFinish") && data["revealEntitiesOnFinish"].is_array()) {
                for (const auto& v : data["revealEntitiesOnFinish"]) {
                    if (v.is_string()) revealEntitiesOnFinish.push_back(v.get<std::string>());
                }
            }
        }
    };

}
