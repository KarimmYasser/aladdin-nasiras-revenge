#pragma once

#include <glad/gl.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "vertex.hpp"

namespace our {

    #define ATTRIB_LOC_POSITION 0
    #define ATTRIB_LOC_COLOR    1
    #define ATTRIB_LOC_TEXCOORD 2
    #define ATTRIB_LOC_NORMAL   3

    /// One OBJ material group → one draw call with a matching material from JSON.
    struct MeshSubmesh {
        GLuint firstIndex = 0;   ///< Offset into the element buffer (in indices, not bytes)
        GLsizei indexCount = 0; ///< Number of GLuint indices (multiple of 3)
        std::string materialName; ///< tinyobj material name (e.g. mat0)
    };

    class Mesh {
        unsigned int VBO, EBO;
        unsigned int VAO;
        GLsizei elementCount;
        std::vector<MeshSubmesh> submeshes;
        /// Cooked triangle soup for ReactPhysics3D (same indexing as GL). Kept for concave mesh colliders.
        std::vector<float> physicsVertexPositions;
        std::vector<uint32_t> physicsIndices;

    public:
        /// Full constructor: optional per-material draw ranges and physics cook data.
        Mesh(const std::vector<Vertex>& vertices,
             const std::vector<unsigned int>& elements,
             std::vector<MeshSubmesh> submeshesIn = {},
             std::vector<float> physicsVertexPositionsIn = {},
             std::vector<uint32_t> physicsIndicesIn = {});

        [[nodiscard]] GLsizei getElementCount() const { return elementCount; }

        [[nodiscard]] bool hasSubmeshes() const { return !submeshes.empty(); }
        [[nodiscard]] const std::vector<MeshSubmesh>& getSubmeshes() const { return submeshes; }

        [[nodiscard]] bool hasPhysicsTriangleData() const {
            return !physicsVertexPositions.empty() && !physicsIndices.empty();
        }
        [[nodiscard]] const std::vector<float>& getPhysicsVertexPositions() const { return physicsVertexPositions; }
        [[nodiscard]] const std::vector<uint32_t>& getPhysicsIndices() const { return physicsIndices; }
        [[nodiscard]] uint32_t getPhysicsVertexCount() const {
            return static_cast<uint32_t>(physicsVertexPositions.size() / 3);
        }
        [[nodiscard]] uint32_t getPhysicsTriangleCount() const {
            return static_cast<uint32_t>(physicsIndices.size() / 3);
        }

        void drawRange(GLsizei firstIndex, GLsizei indexCount) const {
            if (indexCount <= 0) return;
            glBindVertexArray(VAO);
            const void* offset = reinterpret_cast<const void*>(static_cast<uintptr_t>(firstIndex) * sizeof(GLuint));
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, offset);
            glBindVertexArray(0);
        }

        void draw() const {
            drawRange(0, elementCount);
        }

        ~Mesh() {
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteVertexArrays(1, &VAO);
        }

        Mesh(Mesh const &) = delete;
        Mesh &operator=(Mesh const &) = delete;
    };

}
