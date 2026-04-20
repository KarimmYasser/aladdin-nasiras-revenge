#pragma once

#include <glm/glm.hpp>
#include <json/json.hpp>

#include "glm/detail/type_quat.hpp"
#include "reactphysics3d/mathematics/Quaternion.h"

namespace our {

    // A transform defines the translation, rotation & scale of an object relative to its parent
    struct Transform {
    public:
        glm::vec3 position = glm::vec3(0, 0, 0); // The position is defined as a vec3. (0,0,0) means no translation
        glm::vec3 rotation = glm::vec3(0, 0, 0); // The rotation is defined using euler angles (y: yaw, x: pitch, z: roll). (0,0,0) means no rotation
        glm::vec3 scale = glm::vec3(1, 1, 1); // The scale is defined as a vec3. (1,1,1) means no scaling.

        // This function computes and returns a matrix that represents this transform
        glm::mat4 toMat4() const;
         // Deserializes the entity data and components from a json object
        void deserialize(const nlohmann::json&);

        //helper function for wrapping to rp3d lib
        inline static reactphysics3d::Quaternion fromEulerAnglesToRP3DQuaternion(const glm::vec3& eulerAngles) {
            glm::quat quat = glm::quat(eulerAngles);
            return reactphysics3d::Quaternion(quat.x, quat.y, quat.z, quat.w);
        }
    };

}
