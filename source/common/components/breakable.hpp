#pragma once

#include "../ecs/component.hpp"
#include <string>

namespace our {

    /**
     * @brief Defines an entry for loot that can be dropped by a breakable object.
     */
    struct LootEntry {
        std::string type = "coin"; // "coin", "gem", "apple"
        int value = 1;             // How many items are in the drop
    };

    /**
     * @brief This component allows an entity to be broken by Aladdin's sword.
     * 
     * When broken, it can spawn multiple collectible items.
     */
    class BreakableComponent : public Component {
    public:
        // List of loot to drop
        std::vector<LootEntry> lootItems;
        
        // Visual settings
        std::string brokenMesh = "";   // Optional: Mesh to show after breaking (e.g., debris)

        static std::string getID() { return "Breakable"; }

        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            
            if(data.contains("loot") && data["loot"].is_array()){
                lootItems.clear();
                for(const auto& item : data["loot"]){
                    LootEntry entry;
                    entry.type = item.value("type", "coin");
                    entry.value = item.value("value", 1);
                    lootItems.push_back(entry);
                }
            } else {
                // Backward compatibility for single loot
                LootEntry entry;
                entry.type = data.value("lootType", "coin");
                entry.value = data.value("lootValue", 1);
                if(entry.type != "none") lootItems.push_back(entry);
            }

            brokenMesh = data.value("brokenMesh", brokenMesh);
        }
    };

}
