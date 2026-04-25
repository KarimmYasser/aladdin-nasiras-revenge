#include "mesh.hpp"

namespace our {

    Mesh::Mesh(const std::vector<Vertex>& vertices,
               const std::vector<unsigned int>& elements,
               std::vector<MeshSubmesh> submeshesIn,
               std::vector<float> physicsVertexPositionsIn,
               std::vector<uint32_t> physicsIndicesIn)
        : submeshes(std::move(submeshesIn)),
          physicsVertexPositions(std::move(physicsVertexPositionsIn)),
          physicsIndices(std::move(physicsIndicesIn)) {

        elementCount = static_cast<GLsizei>(elements.size());

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(unsigned int), elements.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(ATTRIB_LOC_POSITION);
        glVertexAttribPointer(ATTRIB_LOC_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        glEnableVertexAttribArray(ATTRIB_LOC_COLOR);
        glVertexAttribPointer(ATTRIB_LOC_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        glEnableVertexAttribArray(ATTRIB_LOC_TEXCOORD);
        glVertexAttribPointer(ATTRIB_LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coord));

        glEnableVertexAttribArray(ATTRIB_LOC_NORMAL);
        glVertexAttribPointer(ATTRIB_LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glBindVertexArray(0);
    }

}
