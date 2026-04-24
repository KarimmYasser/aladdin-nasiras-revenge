#pragma once
#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our {
    /**
     * @brief Component for objects that travel through the world and interact with targets on impact.
     */
    class ProjectileComponent : public Component {
    public:
        float damage = 25.0f;           // Damage dealt to enemies or breakables
        float lifetime = 5.0f;          // Max duration in seconds before auto-destruction
        Entity* owner = nullptr;        // The entity that spawned this projectile
        bool destroyedOnImpact = true;  // Whether to remove the entity after one hit

        static std::string getID() { return "Projectile"; }

        void deserialize(const nlohmann::json& data) override {
            if(!data.is_object()) return;
            damage = data.value("damage", damage);
            lifetime = data.value("lifetime", lifetime);
            destroyedOnImpact = data.value("destroyedOnImpact", destroyedOnImpact);
        }
    };
}
